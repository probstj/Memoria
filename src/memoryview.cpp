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

#include "memoryview.h"


// Return random uint between min and max (inclusive)
// srand(uint seed) needs to be called before.
unsigned int get_random_in_interval(unsigned int min, unsigned int max)
// from http://stackoverflow.com/questions/2509679/how-to-generate-a-random-number-from-within-a-range
{
    uint r;
    const unsigned int range = fmin(max - min + 1, RAND_MAX);
    if (range <= 1)
        return min;
    const unsigned int buckets = (uint(RAND_MAX) + 1) / range;
    const unsigned int limit = buckets * range;
    
    /* Create equal size buckets all in a row, then fire randomly towards
     * the buckets until you land in one of them. All buckets are equally
     * likely. If you land off the end of the line of buckets, try again. */
    do
    {
        r = rand();
    } while (r >= limit);
    
    return min + (r / buckets);
}

// Shuffles array in-place using the Fisher–Yates shuffle.
// If only the first subset of elements in the final array are of interest, 
// set only_first to the number of interesting elements.
static void shuffle_array(uint* array, const uint length, const uint only_first = 0){
    uint temp, j;
    uint end = only_first == 0 ? length : only_first;
    for (uint i = 0; i < end; i++) {
        j = get_random_in_interval(i, length - 1);
        temp = array[j];
        array[j] = array[i];
        array[i] = temp;
    }
}

MemoryView::MemoryView(const int bordersize, const QString backside_filename, 
                       const double zoom_factor, QWidget *parent) 
: QGraphicsView(parent), _bordersize(bordersize), _zoom_factor(zoom_factor)
{
    _cols = 0;
    _rows = 0;
    _tiles = NULL;
    _currently_revealed_tiles[0] = NULL;
    _currently_revealed_tiles[1] = NULL;
    _num_clicked_tiles = 0;
    _tileImageHandler = NULL;
    _imageLoaderThread = NULL;
    _interaction_enabled = false;
    _hide_tiles_next_click = false;
    _remove_tiles_next_click = false;
    _num_moving_tiles = 0;
    _num_pairs = 0;
    _found_pairs = 0;
    _boundary_width = 0;
    _boundary_height = 0;
    _elapsed_milliseconds = 0;
    
    _the_scene = new QGraphicsScene(this);
    this->setScene(_the_scene);
    // The background color is set again in resize_images, but here for initializing:
    _the_scene->setBackgroundBrush(QBrush("#0d913b"));
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _raw_backside_image = QImage(backside_filename);
    if (_raw_backside_image.isNull())
        QMessageBox::warning(this, QCoreApplication::applicationName(),
                             tr("WARNING: Failed to open backside image file %1").arg(backside_filename));
    
    _status_text_font.setFamily("DejaVu Sans");
    _status_text_font.setPixelSize(100);
    _status_font_height_factor = 100.0 / QFontMetrics(_status_text_font).height();
    // now, we can set the font height with setPixelSize(height * _status_font_height_factor)

    _score_text_font.setFamily("DejaVu Sans Mono");
    _score_text_font.setPixelSize(100);
    _score_font_height_factor = 100.0 / QFontMetrics(_score_text_font).height();
    // now, we can set the font height with setPixelSize(height * _score_font_height_factor)

    QSettings settings;
    settings.beginGroup("Appearance");
    _status_text_font_height = settings.value("status_text_height", 26).toInt();
    _score_text_font_height = settings.value("score_text_height", 18).toInt();
    settings.endGroup();
        
    _status_text_item = new QGraphicsSimpleTextItem();
    _status_text_item->hide();
    _status_text_item->setBrush(QBrush("black"));
    _the_scene->addItem(_status_text_item);
    _timer_text_item = NULL;
}

MemoryView::~MemoryView()
{
    clear();
    delete _status_text_item;
}

void MemoryView::clear()
{
    if (_imageLoaderThread && _imageLoaderThread->isRunning()) {
        // ImageLoaderThread is still running from previous call to set_images. Stop it first:
        _tileImageHandler->cancelLoading();
        int time_out = 100;
        while (_imageLoaderThread->isRunning() && time_out > 0) {
            // process events, so the canceled tileImageHandler can emit finishedLoading which
            // in turn stops the thread:
            QCoreApplication::processEvents();
            _imageLoaderThread->wait(100);
            time_out--;
        }
        _imageLoaderThread->exit();
    }
    
    for (uint i = 0; i < _cols; i++) {
        for (uint j = 0; j < _rows; j++)
            // the_scene->clear() also deletes items, but not those removed by 
            // the_scene->removeItem() (used e.g. in removePair()), 
            // so better delete all items:
            delete _tiles[i][j];
        delete[] _tiles[i];
    }
    delete[] _tiles;
    _tiles = NULL;
    // now that all tiles have been deleted, we can also delete the image handler:
    delete _tileImageHandler;
    _tileImageHandler = NULL;
    delete _imageLoaderThread;
    _imageLoaderThread = NULL;
        
    // delete previous score items:
    for (int i = 0; i < _score_text_items.size(); ++i) {
        delete _score_text_items.at(i);
    }
    _score_text_items.clear();
    stopTimer();
    delete _timer_text_item;
    _timer_text_item = NULL;
        
    // do not delete _status_text_item when _the_scene->clear() is called:
    _the_scene->removeItem(_status_text_item);
    // All items should be removed now, but just to make sure there are no references left:
    _the_scene->clear();
    _the_scene->addItem(_status_text_item);
    _cols = 0;
    _rows = 0;
    _currently_revealed_tiles[0] = NULL;
    _currently_revealed_tiles[1] = NULL;
    _num_clicked_tiles = 0;
    _num_moving_tiles = 0;
    _num_pairs = 0;
    _found_pairs = 0;
    _remove_tiles_next_click = false;
    _hide_tiles_next_click = false;
}

bool MemoryView::set_images(const uint num_pairs, const uint cols, const uint rows, const QStringList& filenames)
{
    uint num_positions = num_pairs * 2;
    uint available_cards = filenames.count();
    if (num_pairs > available_cards || available_cards == 0) { 
        QMessageBox::warning(this, QCoreApplication::applicationName(),
                             tr("Error: set_images: not enough filenames specified"));
        return false;
    }
    if (num_positions > cols * rows){
        QMessageBox::warning(this, QCoreApplication::applicationName(),
                             tr("Error: set_images: number of cards is greater than available positions (num_pairs > cols*rows)"));
        return false;
    }
    
    // Loading images takes some time. Save and block user interaction status:
    bool interact = _interaction_enabled;
    enableUserInteraction(false);
    
    // clear all previous tiles and arrays:  
    clear();
    _num_pairs = num_pairs;
    
    // show empty board during lenghty image loading time:
    // - not neccessary anymore, because image loading is done in different threads,
    // now, the board responses quickly after setting images.
    //QCoreApplication::processEvents();
    
    _cols = cols;
    _rows = rows;
    
    // First, choose which cards to use.
    // Make a list of available card indexes:
    uint cardindexes[available_cards];
    for (uint i = 0; i < available_cards; i++)
        cardindexes[i] = i;
    
    // Then, shuffle it (only the first 'num_pairs' are of interest):
    shuffle_array(cardindexes, available_cards, num_pairs);
    // We will use the first num_pairs cards.
    
    // initialize all final indexes to the first num_pairs cards, using each card twice:
    uint indexlist[num_positions];
    for (uint i = 0; i < num_pairs; i++) {
        indexlist[2 * i] = cardindexes[i];
        indexlist[2 * i + 1] = cardindexes[i];
    }
    
    // Finally, shuffle this list:
    shuffle_array(indexlist, num_positions);

    // create the TileImageHandler, which will load and distribute the images to the tiles:
    // (can't have a parent, because it will later be moved to another thread)
    _tileImageHandler = new TileImageHandler(num_pairs, 2);
    
    // initialize tile matrix:
    _tiles = new Tile **[_cols];
    for (uint i = 0; i < _cols; ++i)
        _tiles[i] = new Tile *[_rows];
    
    uint idx = 0; // index for 1D array indexlist
    // add new tiles, row-wise until all tiles are distributed:
    for (uint j = 0; j < _rows; ++j) {
        for (uint i = 0; i < _cols; ++i) {
            if (idx < num_positions) {
                _tiles[i][j] = new Tile(indexlist[idx], QPoint(i, j), 
                                       _bordersize, _zoom_factor);
                _tiles[i][j]->setSize(QSize(1, 1));
                _tiles[i][j]->setPos(-10, -10);
                // connect to private slots:
                connect(_tiles[i][j], SIGNAL(tileClicked(Tile*)),
                        this,  SLOT(tileClicked(Tile*)));
                connect(_tiles[i][j], SIGNAL(tileFlipped(Tile*)),
                        this,  SLOT(tileFlipped(Tile*)));
                // add this tile to the image handler:
                _tileImageHandler->addTile(indexlist[idx], filenames.at(indexlist[idx]), _tiles[i][j]);
                
                _the_scene->addItem(_tiles[i][j]);
            } else {
                _tiles[i][j] = NULL;
            }
            idx++;
        }
    }    
    
    // set correct size and positions for all tiles:
    resize_images();
    
    // now, we can start loading the images
    // load them in a separate thread as not to block the GUI:
    _imageLoaderThread = new QThread(this);
    _tileImageHandler->moveToThread(_imageLoaderThread);
    connect(_imageLoaderThread, SIGNAL(started()), _tileImageHandler, SLOT(startLoading()));
    connect(_tileImageHandler, SIGNAL(finishedLoading(bool)), _imageLoaderThread, SLOT(quit()));
    connect(_tileImageHandler, SIGNAL(finishedLoading(bool)), this, SLOT(finishedLoading(bool)));
    _imageLoaderThread->start();
    
    // restore user interaction:
    enableUserInteraction(interact);
    
    emit boardReady();
    
    return true;
}

void MemoryView::enableUserInteraction(const bool enabled)
{
    _interaction_enabled = enabled;
}

void MemoryView::showStatusText(const QString& text)
{
    if (text.isEmpty())
        return hideStatusText();
    _status_text_item->setText(text);
    calc_status_text_size();
    _status_text_item->show();
}

void MemoryView::hideStatusText()
{
    _status_text_item->setText("");
    _status_text_item->hide();
}

void MemoryView::setupPlayers(const QStringList& players)
{
    // delete previous score items:
    for (int i = 0; i < _score_text_items.size(); ++i) {
        delete _score_text_items.at(i);
    }
    _score_text_items.clear();
    
    QString strPairs = tr("pairs:");
    QString strFails = tr("mistakes:");
    _score_text_font.setPixelSize(_score_text_font_height * _score_font_height_factor);
    _score_text_font.setUnderline(true);
    QFontMetrics fm(_score_text_font);
    int width1;
    int width2 = qMax<int>(fm.width(strPairs + " "), fm.width(strFails + " "));
    int width3 = fm.width("0000");
    _score_num_digits = log10(_num_pairs * 3) + 1;
    QString str;
    QTextStream stream(&str);
    stream << qSetPadChar(' ') << qSetFieldWidth(_score_num_digits) << 0;
    str = str % "\n" % str;
    int x = 0;
    
    _score_text_items.reserve(players.length());
    for (int i = 0; i < players.length(); ++i) {
        QGraphicsSimpleTextItem* textitem = new QGraphicsSimpleTextItem(players.at(i));
        _score_text_items.append(textitem);
        textitem->setBrush(QBrush("black"));
        _score_text_font.setUnderline(true);
        textitem->setFont(_score_text_font);
        textitem->setPos(20 + x, 20);
        textitem->show();
        _the_scene->addItem(textitem);
        
        _score_text_font.setUnderline(false);
        textitem = new QGraphicsSimpleTextItem(strPairs + "\n" + strFails, _score_text_items.at(i));
        width1 = fm.width(players[i]) + _score_text_font_height / 2;
        textitem->setPos(width1, 0);
        textitem->setFont(_score_text_font);
        textitem = new QGraphicsSimpleTextItem(str, _score_text_items.at(i));
        textitem->setPos(width1 + width2, 0);
        textitem->setFont(_score_text_font);
        
        x += width1 + width2 + width3 + _score_text_font_height;
    }
}

void MemoryView::updateCurrentScore(const int *found_pairs, const int *failed)
{
    if (_score_text_items.isEmpty())
        return;
    for (int i = 0; i < _score_text_items.size(); ++i) {
        QGraphicsSimpleTextItem* item = (QGraphicsSimpleTextItem*)_score_text_items.at(i)->childItems()[1];
        QString str;
        QTextStream stream(&str);
        stream << qSetPadChar(' ') << qSetFieldWidth(_score_num_digits) << found_pairs[i];
        stream << qSetFieldWidth(0) << endl << qSetFieldWidth(_score_num_digits) << failed[i];
        item->setText(str);
        _score_text_items.at(i)->show();
    }
}

void MemoryView::hideCurrentScore()
{
    for (int i = 0; i < _score_text_items.size(); ++i) {
        _score_text_items.at(i)->hide();
    }
}

void MemoryView::showPlayingTime(const bool startTime)
{
    QString timertext = tr("Time:");
    _timer_text_item = new QGraphicsSimpleTextItem(timertext);
    _score_text_font.setPixelSize(_score_text_font_height * _score_font_height_factor);
    QFontMetrics fm(_score_text_font);
    _timer_text_item->setBrush(QBrush("black"));
    _timer_text_item->setFont(_score_text_font);
    QGraphicsSimpleTextItem* tt = new QGraphicsSimpleTextItem("00:00", _timer_text_item);
    tt->setBrush(_timer_text_item->brush());
    tt->setFont(_score_text_font);
    tt->setPos(fm.width(timertext), 0);
    _timer_text_item->setPos(width() - 38 - _timer_text_item->boundingRect().width() - tt->boundingRect().width(), 20);
    _timer_text_item->show();
    _the_scene->addItem(_timer_text_item);
    _elapsed_milliseconds = 0;
    if (startTime)
        startTimer();
}

void MemoryView::startTimer()
{
    if (!_timer_text_item)
        showPlayingTime();
    _elapsed_milliseconds = 0;
    _timing.start();
    _timer.start(1000, this);
}

void MemoryView::stopTimer()
{
    if (_timer.isActive()) {
        _timer.stop();
        _elapsed_milliseconds += _timing.elapsed();
    }
}

void MemoryView::continueTimer()
{
    if (_elapsed_milliseconds == 0)
        return startTimer();
    _timing.start();
    _timer.start(1000, this);
}

void MemoryView::hidePlayingTime()
{
    if (_timer_text_item){
        stopTimer();
        delete _timer_text_item;
        _timer_text_item = NULL;
    }
}

QTime MemoryView::getPlayingTime() const
{
    if (_timer.isActive())
        return QTime().addMSecs(_elapsed_milliseconds + _timing.elapsed());
    else
        return QTime().addMSecs(_elapsed_milliseconds);

}

void MemoryView::revealTile(const uint column, const uint row)
{
    if (!is_board_ready() || _num_clicked_tiles == 2)
        // A tile can only be revealed this way if no other tile is currently moving (flipping)
        // and there are not already 2 tiles turned over.
        return;
    
    if (column < _cols && row < _rows && _tiles[column][row]) {
        Tile *tile = _tiles[column][row];
        if (!tile->is_flipped())
            // tile is already revealed
            return;
        
        _num_clicked_tiles++;
        _num_moving_tiles++;
        tile->flip();
    }
}


void MemoryView::tileClicked(Tile *tile)
{
    if (!_interaction_enabled || _num_clicked_tiles == 2 || !tile->is_flipped())
        return;
    
    _num_clicked_tiles++;
    _num_moving_tiles++;
    tile->flip();
}

void MemoryView::tileFlipped(Tile* tile)
{
    // This slot receives from tile if it stops moving:
    _num_moving_tiles--;
    
    if (!tile->is_flipped()) {

        // tile has been revealed:
        
        uint id = tile->get_id();
        emit tileRevealed(id, (uint)tile->get_pos().x(), (uint)tile->get_pos().y());
        
        if (!_currently_revealed_tiles[0]) {
            _currently_revealed_tiles[0] = tile;
        }
        else {
            // one tile is revealed already
            _currently_revealed_tiles[1] = tile;
            if (_currently_revealed_tiles[0]->get_id() == id) {
                _found_pairs++;
                if (is_game_over())
                    stopTimer();
                emit matchFound(); 
                _remove_tiles_next_click = true;
                // Note: If pairs are not to be removed but should stay visible on the board, 
                // the line above can be removed. But then (and only then) the following three
                // lines must be uncommented:
                //for (int i = 0; i < 2; ++i)
                //    currently_revealed_tiles[i] = 0;
                //num_clicked_tiles = 0;
            }
            else {
                emit matchFailed();
                _hide_tiles_next_click = true;
            }
        }   
    }
    
    if (is_board_ready())
        emit boardReady();
}

void MemoryView::finishedLoading(bool success) {
    if (success)
        emit imagesLoaded();
}

void MemoryView::hideTiles()
{
    if (_currently_revealed_tiles[0] && _currently_revealed_tiles[1]) {
        for (int i = 0; i < 2; ++i) {
            _num_moving_tiles++;
            _currently_revealed_tiles[i]->flip();
            _currently_revealed_tiles[i] = 0;
        }
        _num_clicked_tiles = 0;
    }
}

void MemoryView::removePair()
{
    if (_currently_revealed_tiles[0] && _currently_revealed_tiles[1] &&
        _currently_revealed_tiles[0]->get_id() == _currently_revealed_tiles[1]->get_id()) 
    {
        uint id = _currently_revealed_tiles[0]->get_id();
    
        for (int i = 0; i < 2; ++i)
            _currently_revealed_tiles[i] = 0;
        _num_clicked_tiles = 0;
    
        int num_found = 0;
        for (uint i=0; i < _cols; ++i)
            for (uint j=0; j < _rows; ++j)
                if (_tiles[i][j] && _tiles[i][j]->get_id() == id) {
                    // Remove the tiles from the scene. This will not delete the Tile objects,
                    // but they might still be used somewhere. They will be deleted later with clear()
                    _the_scene->removeItem(_tiles[i][j]);
                    num_found++;
                    if (num_found == 2) 
                        return;
                }
    }
}

void MemoryView::mousePressEvent(QMouseEvent* event)
{
    QGraphicsView::mousePressEvent(event);
    if (_hide_tiles_next_click) {
        _hide_tiles_next_click = false;
        hideTiles();
    }
    if (_remove_tiles_next_click) {
        _remove_tiles_next_click = false;
        removePair();
        // Because pair is removed without animation, 
        // the board is directly ready after this:
        if (is_board_ready())
            emit boardReady();
    }
}

void MemoryView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    // Why is this->size() 6 pixels wider and higher than event->size() on my computer?
    // Since I need this->size() elsewhere where I don't have access to event->size(), I just stick to this->size():
    setSceneRect(0, 0, width() - 8, height() - 8);
    if (_timer_text_item)
        _timer_text_item->setPos(
            width() - 38 - _timer_text_item->boundingRect().width() - _timer_text_item->childItems()[0]->boundingRect().width(), 20); 
    resize_images();
}

void MemoryView::timerEvent(QTimerEvent* event)
{   
    if (event->timerId() == _timer.timerId()) {
        QGraphicsSimpleTextItem* item = (QGraphicsSimpleTextItem*) _timer_text_item->childItems()[0];
        QTime time = QTime().addMSecs(_elapsed_milliseconds + _timing.elapsed());
        if (time.hour() > 0)
            item->setText(time.toString("hh:mm:ss")); 
        else
            item->setText(time.toString("mm:ss")); 
    } else {
        QObject::timerEvent(event);
    }
}

double MemoryView::calc_tile_size(const uint cols, const uint rows) {
    double tilewidth, tileheight;
    int width = size().width() - 8; // substract border
    int height = size().height() - 8; // substract border

    // calculate tile size:

    // Extra space needed for the possible zoom on each side:
    // (_zoom_factor * tilewidth - tilewidth) / 2;
    // Divide width in 2*extra_space + cols * (tilewidth + _bordersize) - _bordersize:
    // width = (_zoom_factor - 1) * tilewidth + (cols - 1) * _bordersize + cols * tilewidth;
    // Solve for tilewidth:
    // tilewidth * (_zoom_factor - 1 + cols) = width - (cols - 1) * _bordersize;
    // (Analog for height)
    tilewidth = (width - (cols - 1)*_bordersize) / (cols + _zoom_factor - 1.0);
    tileheight = (height - (rows - 1)*_bordersize) / (rows + _zoom_factor - 1.0);

    if (tilewidth < tileheight)
        return tilewidth;
    else
        return tileheight;
}

void MemoryView::calculate_best_distribution(int& cols, int& rows, const int& num_cards)
{
    uint width = size().width() - 8;
    uint height = size().height() - 8;
    // calculate c and r such that c*r==num_cards and c/r == width/height:
    double c = sqrt(num_cards * width / (double)height);
    double r = sqrt(num_cards * height / (double)width);
    
    // now round to some close integer (cols, rows)-tuples and choose the
    // tuple with the biggest resulting tile size:
    // (there might be more possiblilities (i.e. other integer tuples close to (c, r), 
    // e.g. using floor and ceil), but the following two options suffice for now.)
    // option 1:
    uint cols1 = round(c);
    uint rows1 = ceil(num_cards / (double)cols1);
    // option 2:
    uint rows2 = round(r);
    uint cols2 = ceil(num_cards / (double)rows2);
    
    // choose bigger resulting tilesize:
    double tilesize1 = calc_tile_size(cols1, rows1);
    double tilesize2 = calc_tile_size(cols2, rows2);
    if (tilesize1 > tilesize2) {
        cols = cols1;
        rows = rows1;
    } else {
        cols = cols2;
        rows = rows2;
    }
}


void MemoryView::resize_images()
{
    if (!_tiles) 
        // images not loaded yet.
        return;
    
    double tilesize = calc_tile_size(_cols, _rows);
    // calculate offset so the tiles are centered horizontally:
    double x_offset = (size().width() - 8 + _bordersize - _cols*(tilesize + _bordersize)) / 2.0;
    _boundary_width = x_offset;
    _boundary_height = 0.5 * tilesize * (_zoom_factor - 1);
    
    prepareBacksideImage(QSize(tilesize, tilesize));
    
    for (uint i = 0; i < _cols; i++) {
        for (uint j = 0; j < _rows; j++)
        {
            if (_tiles[i][j]) {
                _tiles[i][j]->setSize(QSize(tilesize, tilesize));
                _tiles[i][j]->setBacksideImage(&_backside_image);
            
                // set tile's central position:
                _tiles[i][j]->setPos(i * (tilesize + _bordersize) + _boundary_width + 0.5 * tilesize, 
                                     j * (tilesize + _bordersize) + _boundary_height + 0.5 * tilesize);
            }
        }
    }
    
    //QRadialGradient gradient(QPointF(0, 0), this->width());
    //gradient.setColorAt(1, "#0d913b");
    //gradient.setColorAt(0, QColor::fromRgbF(0, 0, 0, 1));
    
    QRadialGradient gradient(QPointF(width() / 2.0, height() / 2.0), fmax(width(), height()));
    //gradient.setColorAt(1, "#043214");
    gradient.setColorAt(1, "black");
    gradient.setColorAt(0, "#0d913b");
    
    this->setBackgroundBrush(QBrush(gradient));
    
    calc_status_text_size();
}

void MemoryView::prepareBacksideImage(QSize tilesize)
{
    _backside_image = QImage(tilesize.width(), tilesize.height(), 
                            QImage::Format_ARGB32_Premultiplied);
    
    // initialize image data with transparent color:
    _backside_image.fill(qRgba(0, 0, 0, 0));
    
    // image bounding rect minus pen width:
    QRectF r(0, 0, tilesize.width() - 1, tilesize.height() - 1);
    
    QPainter painter(&_backside_image);
    //painter.setRenderHint(QPainter::SmoothPixmapTransform);  
    painter.setRenderHint(QPainter::Antialiasing);
    
    painter.setBrush(QBrush("black"));
    painter.drawRoundedRect(r, _bordersize, _bordersize);
    
    // draw image only on solid rounded rect:
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.drawImage(QRect(0, 0, tilesize.width(), tilesize.height()), _raw_backside_image);
    
    // add dark border:
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(r, _bordersize, _bordersize);
}

void MemoryView::calc_status_text_size()
{
    if (_status_text_item->text().isEmpty() || _boundary_height == 0)
        return;
    if (_status_text_font_height > _boundary_height)
        _status_text_font.setPixelSize(_boundary_height * _status_font_height_factor);
    else
        _status_text_font.setPixelSize(_status_text_font_height * _status_font_height_factor);
    _status_text_item->setFont(_status_text_font);
    QFontMetrics fm(_status_text_font);
    int h = fm.height();
    int w = fm.width(_status_text_item->text());
    _status_text_item->setPos((width() - w) / 2, height() - (_boundary_height + h) / 2 - 8);
}


//#include "memoryview.moc"
