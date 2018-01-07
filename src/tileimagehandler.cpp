/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  Juergen <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "tileimagehandler.h"




// A range of indexes starting at start_index with length range_len is shrinked in such a way, that 
// the sum of all data points over the new range is maximized (data_len: length of int *data array). 
// The new range_len will be halfed, the new start_index is the first index of the maximized index 
// range. If the optional parameter roll_to_start_of_range is true, the new range is allowed to 
// wrap over from the end of the old range to its start.
// The supplied range may wrap over to the start of data.
static void shrink_range(int &start_index, int &range_len, const int *data, const int data_len, 
                         const bool roll_to_start_of_range = false){
    int old_start = start_index, old_len = range_len, old_last = old_start + old_len - 1;
    range_len = old_len / 2;
    
    // Integrate leftmost part of old area:
    int sum = 0;
    for (int i = old_start; i < old_start + range_len; i++)
        if (i < data_len)
            sum += data[i];
        else
            // roll over to beginning of data array:
            sum += data[i - data_len];
        
        int max_sum = sum;
    // shift the integration area:
    for (int i = old_start + 1; i <= (roll_to_start_of_range ? old_last : old_last - range_len + 1); i++) {
        // index of data to substract:
        int sub_index = i - 1;
        if (sub_index >= data_len)
            // roll over to beginning of data array:
            sub_index -= data_len;
        // index of data to add:
        int add_index = i + range_len - 1;
        if (roll_to_start_of_range && add_index > old_last)
            // roll over to beginning of old array:
            add_index -= old_len;
        if (add_index >= data_len)
            // roll over to beginning of data array:
            add_index -= data_len;
        sum = sum - data[sub_index] + data[add_index];
        if (sum > max_sum) {
            max_sum = sum;
            if (i >= data_len)
                start_index = i - data_len;
            else
                start_index = i;
        }
    }
}

static QColor get_most_prominent_hue(const QImage &image)
{
    //return QColor("green");
    const QRgb *scanline;
    int hue_count[360] = {0}; 
    bool monochrome = true;
    int h, s, v;
    for (int i = 0; i < image.height(); i++) {
        scanline = (const QRgb*) image.constScanLine(i);
        for (int j = 0; j < image.width(); j++) {
            QColor color(scanline[j]);
            color.getHsv(&h, &s, &v);
            //h = color.hue();
            if (h != -1) {
                hue_count[h]++;
                if (monochrome)
                    monochrome = false;
            }
        }
    }
    if (monochrome)
        return QColor("black");
    
    int current_start_at = 0, current_num_candidates = 360;
    // decrease number of candidates until only a few are left over:
    shrink_range(current_start_at, current_num_candidates, hue_count, 360, true);
    //printf("start %i, range %i\n", current_start_at, current_num_candidates);
    while (current_num_candidates > 5) {
        // Integrate hue_count over current_num_candidates/2 neigboring values, starting at current_start_at.
        // Shift the integration area stepwise until the whole area has been probed in this way.
        // The area with the highest integrated counts wins.
        // This is calculated in this way so that larger ranges with a high amount of similar hue 
        // are prefered over single high peaks of a very small hue range.
        shrink_range(current_start_at, current_num_candidates, hue_count, 360, false);
        //printf("start %i, range %i\n", current_start_at, current_num_candidates);
    }
    
    // find maximum of last points:
    int hue = -1, max_count = -1;
    for (int i = current_start_at; i < current_start_at + current_num_candidates; i++) {
        int idx = i;
        if (idx >= 360)
            // roll over to beginning of data array:
            idx -= 360;
        if (hue_count[idx] > max_count) {
            max_count = hue_count[idx];
            hue = idx;
        }
    }
    printf("final hue: %i\n\n", hue);
    return QColor::fromHsv(hue, 255, 255);
}


inline static int rgb2key(int r, int g, int b, int bitshift) {
    //return 1;
    return (((r >> bitshift) << 16) + 
    ((g >> bitshift) <<  8) + 
    (b >> bitshift) );
}

inline static int favorHue(int r, int g, int b) {
    return (abs(r-g)*abs(r-g) + abs(r-b)*abs(r-b) + abs(g-b)*abs(g-b)) / 1000 + 1; 
}

// Algorithm from Pieroxy <pieroxy@pieroxy.net>,
// more details here: http://pieroxy.net/blog/pages/color-finder/index.html 
static int find_most_prominent_equivalence_class(const QHash<int, pixeldata_t> data, const int bitshift,
                                                 const int allowed_key=0, const int allowed_key_bitshift=8) {
    QHash<int, int> eqclass_counts;
    int r, g, b, key = 0, count;
    // scan through all pixels:
    QHash<int, pixeldata_t>::const_iterator di;
    for (di = data.constBegin(); di != data.constEnd(); ++di) {
        // get rgb values:
        r = di.value().r;
        g = di.value().g;
        b = di.value().b;
        // check whether pixel belongs to allowed equivalence class allowed_key 
        // by shifting it with corresponding allowed_key_bitshift:
        if (rgb2key(r, g, b, allowed_key_bitshift) == allowed_key) {
            // pixel is allowed, so designate a new equivalence key:
            key = rgb2key(r, g, b, bitshift);
            if (key != 0) { // discard very dark color
                // add to the count for this key:
                count = di.value().weight * di.value().count;
                eqclass_counts.insert(key, eqclass_counts.value(key, 0) + count);
            }
        }
    }
    
    // iterate through equivalence class counts and return most prominent member's key:
    QHash<int, int>::const_iterator i;
    count = 0;
    for (i = eqclass_counts.constBegin(); i != eqclass_counts.constEnd(); ++i) {
        if (i.value() > count) {
            count = i.value();
            key = i.key();
        }
    }
    return key;
}

// Algorithm from Pieroxy <pieroxy@pieroxy.net>,
// more details here: http://pieroxy.net/blog/pages/color-finder/index.html                
static QColor get_most_prominent_color(const QImage &image, const int max_pixel = 5000) {
    const QRgb *rgbdata;
    QHash<int, pixeldata_t> data;
    pixeldata_t pd;
    int key;
    int pixelcount = image.width() * image.height();
    // we don't need to count all pixels, so skip some:
    int skip = pixelcount < max_pixel ? 1 : pixelcount / max_pixel;

    QRgb pixel;
    int r, g, b;
    bool monochrome = true;
    rgbdata = (const QRgb*) image.constBits();
    // count the occurences of rgb-triplets and save in data:
    for (int i = 0; i < pixelcount; i += skip) {
        // get rgb values:
        pixel = rgbdata[i];
        r = qRed(pixel);
        g = qGreen(pixel);
        b = qBlue(pixel);
        key = rgb2key(r, g, b, 0);
        if (data.contains(key)){
            pd = data.value(key);
            pd.count++;
            data[key] = pd;
        }
        else {
            pd.r = r;
            pd.g = g;
            pd.b = b;
            // The count of this color will later be multiplied by this weight factor,
            // which gives us the possibility to tweak the final winning color:
            pd.weight = favorHue(r, g, b);
            pd.count = 1;
            data.insert(key, pd);
            if (r != g || g != b)
                monochrome = false;
        }
    }

    if (monochrome)
        return QColor("black");

    // Divide data in equivalence classes (rightshift the r, g, b values by 6, i.e. integer divide by 64,
    // which results in values between 0 and 3 for each component. Colors are in one equivalence class
    // if they have the same right-shifted components.) The equivalence class with the most members wins.
    // When counting the members, their number of occurence is multiplied by their weight factor from above.
    // The returned key denotes this equivalence class:
    key = find_most_prominent_equivalence_class(data, 6);
    // Now, only count the colors in the equivalence class which was most promiment before,
    // but this time rightshift by 4, then by 2 and finally count the colors without shifting:
    key = find_most_prominent_equivalence_class(data, 4, key, 6);
    key = find_most_prominent_equivalence_class(data, 2, key, 4);
    key = find_most_prominent_equivalence_class(data, 0, key, 2);
    // key identifies the most prominent color. Transform to rgb-triplet:
    r = key >> 16;
    key -= (r << 16);
    g = key >> 8;
    key -= (g << 8);
    b = key;
    //printf("result: %i, %i, %i\n", r, g, b);
    return QColor(r, g, b);
}



TileImageHandler::TileImageHandler(const uint num_images, const uint num_tiles_per_image, QObject* parent) : 
QObject(parent), _numImages(num_images), _numTilesPerImage(num_tiles_per_image)
{
    _fnames.reserve(num_images);
    _images = new QImage*[num_images];
    for (uint i = 0; i < num_images; ++i)
        _images[i] = NULL;
    _tiles = new Tile*[num_images * num_tiles_per_image];
    for (uint i = 0; i < num_images * num_tiles_per_image; ++i)
        _tiles[i] = NULL;
    _countIdsAdded = 0;
    _countTilesAdded = 0;
    _loadingCanceled = false;
    _loadingSuccessful = false;
}

TileImageHandler::~TileImageHandler()
{
    //just delete the refence to the tiles, not the tiles themselves:
    delete[] _tiles; 
    // TileImageHandler owns the images, so delete all:
    for (uint i = 0; i < _numImages; ++i)
        delete _images[i];
    delete[] _images; 
    _fnames.clear();
    _id_to_index.clear();
}


void TileImageHandler::addTile(const uint id, const QString& filename, Tile* tile)
{
    if (_id_to_index.contains(id)) {
        // a tile with the same id has been added already
        uint idx = _id_to_index.value(id);
        for (uint i = 1; i < _numTilesPerImage; ++i)
            if (!_tiles[idx * _numTilesPerImage + i]) {
                _tiles[idx * _numTilesPerImage + i] = tile;
                _countTilesAdded++;
                return;
            }
            printf("Error: Too many tiles with the same id specified! Ignoring additional tiles.\n");
        return;
    } else {
        if (_countIdsAdded == _numImages) {
            printf("Warning: addTile(): Already added all images. This tile is ignored.\n");
            printf("Did you specify the right amount of images in the constructor?\n");
            return;
        }
        // first time a tile with this id is added
        _id_to_index.insert(id, _countIdsAdded);
        _fnames.append(filename);
        _tiles[_countIdsAdded * _numTilesPerImage] = tile;
        _countIdsAdded++;
        _countTilesAdded++;
    }
}

void TileImageHandler::cancelLoading()
{
    _loadingCanceled = true;
}


void TileImageHandler::startLoading()
{
    if (_loadingSuccessful){
        printf("Warning: startLoading() called although all images have been "
               "loaded already. Ignoring function call.\n");
        return;
    }

    // check: all tiles must have been added before calling this
    if (_countTilesAdded < _numImages * _numTilesPerImage) {
        printf("Warning: startLoading() cannot be called before all tiles "
               "have been added with addTile(). Loading will not start.\n");
        return;
    }

    for (uint i = 0; i < _numImages; ++i) {
        // this takes some time:
        if (!_images[i])
            _images[i] = new QImage(_fnames[i]);
        if (_loadingCanceled) {
            break;
        }
        if (_images[i]->isNull())
            printf("WARNING: Failed to open file %s\n", _fnames[i].toStdString().c_str());
        
        //QColor bordercolor = get_most_prominent_hue(iQColormage);
        //QColor bordercolor = get_average_color(image);
        //QColor bordercolor = get_most_prominent_color_slow(image);
        QColor bordercolor = get_most_prominent_color(*_images[i]);
        
        // after one image has been loaded, distribute this image to all tiles with the same id:
        for (uint j = 0; j < _numTilesPerImage; ++j)
            // Calling Tile::setImage method directly does not work, since it will be executed in the current thread:
            //tiles[i * numTilesPerImage + j]->setImage(images[i], bordercolor);
            // setImage calls update() of the Tile, this should be executed in the GUI thread. To achieve this,
            // connect with a QueuedConnection, emit a signal and disconnect again:
            connect(this, SIGNAL(sendImage(QImage*,QColor)), _tiles[i * _numTilesPerImage + j], SLOT(setImage(QImage*,QColor)),
                    Qt::QueuedConnection
            );
        // note: the Tile objects do not take ownership of the images
        
        // send the loaded image and the color to the tiles:
        emit sendImage(_images[i], bordercolor);
        // and disconnect the signal again:
        for (uint j = 0; j < _numTilesPerImage; ++j)
            disconnect(this, SIGNAL(sendImage(QImage*,QColor)), _tiles[i * _numTilesPerImage + j], SLOT(setImage(QImage*,QColor)));
    }
    
    _loadingSuccessful = !_loadingCanceled;
    if (_loadingSuccessful) {
        // clear file names and refences to tiles: we don't need them anymore

        // just delete the refence to the tiles, not the tiles themselves:
        delete[] _tiles;
        _tiles = NULL;

        _fnames.clear();
        _id_to_index.clear();

        // TileImageHandler owns the QImages, which are still used by the tiles.
        // They will be deleted with ~TileImageHandler.
    }

    // finished loading all files:
    emit finishedLoading(_loadingSuccessful);
    
    
}

//#include "tileimagehandler.moc"
