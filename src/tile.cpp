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

#include "tile.h"

Tile::Tile(const uint id, const QPoint position, const int bordersize, const double zoom_factor, 
           QGraphicsItem* parent)
: QGraphicsObject(parent), _id(id), _position(position), _bordersize(bordersize), _max_zoom(zoom_factor)
{
    // create dummy Image:
    _image = new QImage();
    _flipped = true;
    _flipping_angle = 0;
    _current_size = _size = QSize(0, 0);
    _current_scaling_value = _scaling_value = 0.5;
    _bordercolor = QColor("white");
    _backside_image = NULL;
    _zoom_factor = 0;
    
    _last_mouse_coords.setX(0);
    _last_mouse_coords.setY(0);
    setAcceptHoverEvents(true);
}

Tile::~Tile()
{
    if (_timer.isActive())
        _timer.stop();
}


void Tile::setSize(const QSize& newSize)
{
    _current_size = _size = newSize;
    calcImageRects();
}

QRectF Tile::boundingRect() const
{
    if (_flipping_angle)
        // make it bigger than it actually is, because the size increases slightly during flipping:
        return QRectF(-0.75 * _current_size.width(), -0.75 * _current_size.height(), 
                      1.5 * _current_size.width(), 1.5 * _current_size.height());
    else
        return QRectF(-0.5 * _current_size.width(), -0.5 * _current_size.height(), 
                      _current_size.width(), _current_size.height());
}

void Tile::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    (void) option; // suppress unused-parameter warning
    (void) widget; // suppress unused-parameter warning

    // much nicer images, especially if up-scaled:
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->setRenderHint(QPainter::Antialiasing);
    
    if (_flipping_angle) {
        QTransform transform = painter->transform();
        transform.rotate(_flipping_angle, Qt::YAxis);
        painter->setTransform(transform);
    }
    
    // boundingRect minus pen width:
    QRectF r(-0.5 * _current_size.width(), 
             -0.5 * _current_size.height(), 
             _current_size.width() - 1, 
             _current_size.height() - 1);
    
    if (_flipped) { 
        if (_backside_image)
            painter->drawImage(
                QRectF(-0.5 * _current_size.width(), -0.5 * _current_size.height(),
                       _current_size.width(), _current_size.height()), 
                *_backside_image);
        else {
            // just in case...
            painter->setBrush(QBrush("darkGray"));
            painter->drawRoundedRect(r, _bordersize, _bordersize);
        }
    }
    else {
        //QLinearGradient gradient(r.topLeft(), r.bottomRight());
        QRadialGradient gradient(QPointF(0, 0), _current_size.height() * 1.4, _last_mouse_coords);
        gradient.setColorAt(1, _bordercolor);
        gradient.setColorAt(0.5, _bordercolor);
        gradient.setColorAt(0, QColor::fromRgbF(1, 1, 1, 1));
        painter->setBrush(QBrush(gradient));
        painter->drawRoundedRect(r, _bordersize, _bordersize);
        if (_image->isNull())
            //:This text is shown on a tile while the tile image loads.
            painter->drawText(r, Qt::AlignCenter, tr("Please\nwait..."));
        else
            painter->drawImage(_image_destination_rect, *_image, _image_source_rect);
    }
    //TODO following only for debug:
    //painter->drawText(r, Qt::AlignBottom, QString::number(get_id()));
}

void Tile::setBacksideImage(const QImage* newImage)
{
    _backside_image = newImage;
}

void Tile::setImage(QImage* img, const QColor bordercolor)
{
    // delete previous (dummy) image:
    delete _image;
    _image = img;
    _bordercolor = bordercolor;
    calcImageRects();
    if (!_flipped && !_flipping_angle) {
        update();
    }
}


void Tile::setScalingValue(const double factor)
{
    if (factor < 0.0 || factor > 1.0) {
        printf("Warning: setScalingValue: factor out of bounds!\n");
        if (factor < 0.0)
            _scaling_value = 0;
        else
            _scaling_value = 1;
    }
    else
        _scaling_value = factor;
    _current_scaling_value = _scaling_value;
    calcImageRects();
}

void Tile::flip()
{
    _timerstep = 0;
    _timer.start(40, this);
}

void Tile::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (!_flipping_angle && event->button() == Qt::LeftButton){
        emit tileClicked(this);
    }
}

void Tile::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    _last_mouse_coords = event->pos();
    calcZoomFactor(event->pos());
    calcZoomedSize();
    calcImageRects();
    update();
}

void Tile::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    (void) event; // suppress unused-parameter warning

    // move to top:
    setZValue(100);
}

void Tile::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    (void) event; // suppress unused-parameter warning

    // reset z value:
    setZValue(0);
    prepareGeometryChange();
    _current_scaling_value = _scaling_value;
    _current_size = _size;
    calcImageRects();
}

void Tile::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == _timer.timerId()) {
        prepareGeometryChange();
        _flipping_angle -= 18;
        _timerstep++;
        if (_timerstep == 5) {
            // halfway through
            // now, flipping_angle is -90
            // go to +90 degrees so the final image is not mirrored:
            _flipping_angle = 90;
            // paint the other side of the card:
            _flipped = !_flipped;
        }
        if (_timerstep > 9) {
            // finished
            _flipping_angle = 0;
            _timer.stop();
            emit tileFlipped(this);
        }
        calcZoomedSize();
        calcImageRects();
        update();   
    } else {
        QObject::timerEvent(event);
    }
}

void Tile::calcZoomFactor(QPointF mousePos)
{
    double outside_percentage = 0.95, inside_percentage = 0.1;
    double transitionwidth = (outside_percentage - inside_percentage);
    double wh = _size.width() / 2.0, hh = _size.height() / 2.0;
    int x = (int) fabs(mousePos.x());
    int y = (int) fabs(mousePos.y());
    _zoom_factor = x <= inside_percentage * wh ? 1.0 : (
        x > outside_percentage * wh ? 0.0 : (
            0.5 * (cos(PI*(x/wh - inside_percentage)/transitionwidth) + 1.0)));
    _zoom_factor*= y <= inside_percentage * hh ? 1.0 : (
        y > outside_percentage * hh ? 0.0 : (
            0.5 * (cos(PI*(y/hh - inside_percentage)/transitionwidth) + 1.0)));
}

void Tile::calcZoomedSize()
{
    _current_scaling_value = (1.0 - _scaling_value) * _zoom_factor + _scaling_value;
    
    // Find maximum zooming value (zoom value if mouse pointer is in the center):
    // This depends on whether we see the front or the back (backside has max zoom of 1.1)
    // and there is also a transition during flipping.
    double zoom;
    if (_flipping_angle) {
        // currently flipping -> we need to calculate transition
        if ((_flipped && _timerstep < 5) || (!_flipped && _timerstep >= 5)) {
            // transition from backside to front (i.e. from 1.1 to max_zoom)
            zoom = (_max_zoom - 1.1) * _timerstep / 10.0 + 1.1;
        }
        else {
            // transition from front side to back (i.e. from max_zoom to 1.1)
            zoom = (1.1 - _max_zoom) * _timerstep / 10.0 + _max_zoom;
        }
    }
    else
        zoom = _flipped ? 1.1 : _max_zoom;
    
    int maxwidth = _size.width() * zoom;
    int maxheight = _size.height() * zoom;
    prepareGeometryChange();
    _current_size.setWidth((maxwidth - _size.width()) * _zoom_factor + _size.width());
    _current_size.setHeight((maxheight - _size.height()) * _zoom_factor + _size.height());
}

void Tile::calcImageRects()
{
    double scaleH = (_current_size.width() - 2*_bordersize) / double(_image->width());
    double scaleV = (_current_size.height() - 2*_bordersize) / double(_image->height());
    double scale_min = fmin(scaleH, scaleV);
    double scale_max = fmax(scaleH, scaleV);
    
    // scaling goes linearly from scale_max to scale_min with current_scaling_value 0..1:
    double scaling = (scale_min - scale_max) * _current_scaling_value + scale_max;
    
    double src_width = fmin(_image->width(), (_current_size.width() - 2*_bordersize) / scaling);
    double src_height = fmin(_image->height(), (_current_size.height() - 2*_bordersize) / scaling);
    double dst_width = fmin(_current_size.width() - 2*_bordersize, _image->width() * scaling);
    double dst_height = fmin(_current_size.height() - 2*_bordersize, _image->height() * scaling);
    _image_source_rect.setRect(
        (_image->width() - src_width) / 2.0,
        (_image->height() - src_height) / 2.0,
        src_width,
        src_height
    );
    _image_destination_rect.setRect(
        -0.5 * dst_width,
        -0.5 * dst_height,
        dst_width,
        dst_height
    );
}


// necessary for Qt's meta objectc compiler, e.g. for signal-slot-system:
//#include "tile.moc"
