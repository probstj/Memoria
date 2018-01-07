/*
 * MIT License
 *
 * Copyright (c) 2014 Jürgen Probst
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef TILEIMAGEHANDLER_H
#define TILEIMAGEHANDLER_H

#include <QObject>
#include "tile.h"

struct pixeldata_t { int r, g, b, weight, count; };

// This class will be run in a separate thread. It loads images from the hdd in the background 
// without blocking the GUI and distributes the loaded QImages to the tiles.
class TileImageHandler : public QObject
{
    Q_OBJECT
    
public:
    TileImageHandler(const uint num_images, const uint num_tiles_per_image, QObject* parent = 0);
    ~TileImageHandler();
    
    void addTile(const uint id, const QString &filename, Tile *tile);
    void cancelLoading();
    
public slots:
    void startLoading();
    
signals:
    void finishedLoading(bool success);
    void sendImage(QImage *img, const QColor bordercolor);
    
private:
    const uint _numImages, _numTilesPerImage;
    uint _countIdsAdded, _countTilesAdded;
    bool _loadingCanceled, _loadingSuccessful;
    QHash<uint, uint> _id_to_index; // map from id number to index of local arrays
    QStringList _fnames; // list of filenames (length: num_images)
    QImage **_images; // array of pointers to loaded images (length: num_images)
    Tile** _tiles; // array of pointers to Tile objects (length: num_tiles_per_imagehe*num_images)
};

#endif // TILEIMAGEHANDLER_H
