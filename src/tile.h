/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  ... <email>
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

#ifndef TILE_H
#define TILE_H

#include <QGraphicsObject>
#include <QBasicTimer>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <stdio.h> // for printf()
#include <math.h>

#define PI 3.1415926535897

class Tile : public QGraphicsObject
{
    // necessary for Qt's meta objectc compiler, e.g. for signal-slot-system:
    Q_OBJECT 
    
public:
    Tile(const uint id, const QPoint position, const int bordersize, const double zoom_factor = 2.5,
         QGraphicsItem *parent = 0);
    
    ~Tile();
    
    void setSize(const QSize &newSize);
    
    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
    
    // All tiles share the same backside image, so it should be created outside of the Tile class and 
    // just referenced here. For best quality, the image should be updated if the tile is resized.
    // The Tile class does not take ownership of the image, it must be deleted outside.
    void setBacksideImage(const QImage *newImage);
    
    // Sets the image scaling value. factor must be between 0.0 and 1.0. A value of 0.0 will scale
    // the image so that it fills the entire tile, cutting a part of the image off if its aspect ratio
    // is not the same than the tile's. A value of 1.0 will scale the image so that it entirely fits
    // inside the tile, keeping the image's aspect ratio, which will result in empty borders next to
    // the image. The default value upon creation of Tile is 0.5, which means a smaller part is cut
    // off of the image than at 0.0 and there are empty borders smaller than the borders at 1.0.
    void setScalingValue(const double factor);
    
    uint get_id() const { return _id; } ;
    bool is_flipped() const { return _flipped; };
    QPoint get_pos() const { return _position; };
    
public slots:
    void flip(); 
    // Multiple tiles also share the same foreground image, so this is also created outside of the 
    // Tile class and just referenced here. No need to update this image during a resize.
    // The Tile class does not take ownership of the image, it must be deleted outside.
    void setImage(QImage *img, const QColor bordercolor);
    
signals:
    void tileClicked(Tile *tile);
    void tileFlipped(Tile *tile);
    
protected:
    virtual void mousePressEvent (QGraphicsSceneMouseEvent *event);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    virtual void timerEvent(QTimerEvent *event);
    
private:
    // Calculates the zoom factor, (see below: double zoom_factor)
    void calcZoomFactor(QPointF mousePos);
    // Calculates the zoomed size of the tile, taking into account the zoom factor (needs to be 
    // updated beforehand with calcZoomFactor during mouse movement), and the current
    // state of the card (flipped or not / transition)
    void calcZoomedSize();
    // Calculates rectangles needed to copy image to the tile, 
    // i.e. part of image that will be cut out (image_source_rect) and 
    // rectangle where this part will be copied to (image_destination_rect):
    void calcImageRects();

    // The unique labels of a card:
    const uint _id; // cards with same image have same id
    const QPoint _position; 

    QImage* _image;
    QRectF _image_destination_rect, _image_source_rect;
    int _bordersize;
    QColor _bordercolor;
    double _max_zoom;
    const QImage *_backside_image;
    
    // if flipped, the card's backside is visible:
    bool _flipped; 
    // while a card is being flipped, flipping_angle turns the card in 10 steps by 180 degrees:
    // (see timerEvent)
    int _flipping_angle;
    // counts to 10 during flipping:
    int _timerstep;
    QBasicTimer _timer;
        
    // During hoverMoveEvent, the tile changes its size and the image scaling value,
    // then the current_ variables below differ from the set values:
    double _scaling_value, _current_scaling_value;
    QSize _size; 
    QSizeF _current_size;
    // I have made the size a QSize (not QSizeF) on purpose, so the supplied backside image (always QSize),
    // won't need to be stretched (except during hovering), which would distort the image.
    
    // This factor (between 0.0 and 1.0) tells how much a card must be zoomed. If the mouse pointer is in
    // the center area of a card, this is 1.0, on the outside border or outside, this is 0.0. 
    // This is independent of the current state of the card (flipped or not).
    double _zoom_factor;
    
    QPointF _last_mouse_coords;
};


#endif // TILE_H
