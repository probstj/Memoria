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
