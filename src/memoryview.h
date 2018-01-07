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

#ifndef MEMORYVIEW_H
#define MEMORYVIEW_H

#include <QGraphicsView>
#include <QThread>
#include <QCoreApplication>
#include <QSettings>
#include <QTextStream>
#include <QStringBuilder>
#include <QTime>
#include <QMessageBox>
#include "tileimagehandler.h"

// Return random uint between min and max (inclusive)
// srand(uint seed) needs to be called before.
unsigned int get_random_in_interval(unsigned int min, unsigned int max);

class MemoryView : public QGraphicsView
{
    // necessary for Qt's meta objectc compiler, e.g. for signal-slot-system:
    Q_OBJECT
    
public:
    MemoryView(const int bordersize, const QString backside_filename, const double zoom_factor = 2.5, 
               QWidget *parent = 0);
    
    ~MemoryView();
    
    // Clears all tiles and deletes all arrays:
    void clear();
    
    // Loads all images from files specified in filenames. Arranges images randomly among columns
    // and rows specified by cols_ and rows_. Returns true only if enough filenames were specified 
    // and all files could be loaded; otherwise returns false.
    bool set_images(const uint num_pairs, const uint cols, const uint rows, const QStringList &filenames);
    
    // User interaction (cards flipped when clicked on) must be activated before:
    // This can be deactivated e.g. during A.I. opponent's move.
    void enableUserInteraction(const bool enabled = true);
    
    // While a tile is in the process of flipping, this returns false, otherwise true:
    bool is_board_ready() const { return _num_moving_tiles == 0 && !_hide_tiles_next_click && !_remove_tiles_next_click; };
    
    bool is_game_over() const { return _num_pairs == _found_pairs; };
    
    // Calculate the number of columns and rows so that the count cards distributed in the matrix are
    // as big as possible, i.e. use the view's current size as efficient as possible:
    void calculate_best_distribution(int &cols, int &rows, const int &num_cards);
    
    void showStatusText(const QString &text);
    void hideStatusText();
    
    // Call this to show the current score:
    void setupPlayers(const QStringList &players);
    // If setupPlayers has been called, this must be called every time the score changes:
    // (The length of the arrays must be the number of players!)
    void updateCurrentScore(const int *found_pairs, const int *failed);
    // Don't show the score anymore: (To show the score again, updateCurrentScore must be called)
    void hideCurrentScore();
    
    void showPlayingTime(const bool startTime = false);
    void startTimer();
    void stopTimer();
    void continueTimer();
    bool isTimerRunning() const { return _timer.isActive(); };
    void hidePlayingTime();
    QTime getPlayingTime() const;
    
public slots:
    // Reveals the tile in column and row, but only if less than two cards are currently revealed. 
    // If this tile is the second revealed tile, it is checked for a match or a fail.
    // This function does the same than when the user clicks on the tile with enabled user interaction.
    // If this function is called while a tile is still flipping, the call is ignored, so it is 
    // best to always wait for the signal tileRevealed, or matchFound/Failed if it was the second tile,
    // or check the function is_board_ready().
    void revealTile(const uint column, const uint row);
    
private slots:
    // The following two slots should be connected to the tiles.
    // This one will flip tiles, but only if less than two are turned over:
    void tileClicked(Tile *tile);
    // This will check for a match after two tiles have been revealed:
    void tileFlipped(Tile *tile);
    // If _tileImageHandler finished loading, connect to this:
    void finishedLoading(bool success);
    
signals: 
    void matchFound();
    void matchFailed(); // TODO: differ between unlucky fail and fail if correct cards should have been known.
    
    // To keep track of revealed cards, e.g. for A.I. opponent:
    void tileRevealed(uint id, uint col, uint row); 
    
    // This is signaled if the next tile is ready to be revealed, i.e. after all tiles have finished flipping 
    // and after tiles have been removed/hidden (following an additional click on the board)
    void boardReady();

    // This is emitted if all tile images have been loaded. Loading is done in the background.
    // boardReady() is emitted before this, i.e. the game can start while the images
    // are still loading.
    void imagesLoaded();
    
protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    virtual void timerEvent(QTimerEvent *event);
    
private:
    // Calculate tile size such that cols columns and rows rows fit in view's current size:
    double calc_tile_size(const uint cols, const uint rows);
    
    // resize all tile images so they fit within view's current size
    void resize_images();
    
    void prepareBacksideImage(QSize tilesize);
    
    // only hides tiles if 2 tiles have been revealed:
    void hideTiles();
    // only removes pair if both cards are currently revealed:
    void removePair();
    
    void calc_status_text_size();
    
    QGraphicsScene *_the_scene;
    QImage _backside_image, _raw_backside_image;
    QThread *_imageLoaderThread;
    TileImageHandler *_tileImageHandler;
    
    QGraphicsSimpleTextItem * _status_text_item;
    QFont _status_text_font;
    // factor for transformation between font height and pixelsize of the status font:
    double _status_font_height_factor;
    int _status_text_font_height;
    
    QVector<QGraphicsSimpleTextItem*> _score_text_items;
    QFont _score_text_font;
    // factor for transformation between font height and pixelsize of the score font:
    double _score_font_height_factor;
    int _score_text_font_height;
    int _score_num_digits;
    
    QGraphicsSimpleTextItem* _timer_text_item;
    QBasicTimer _timer;
    QTime _timing;
    uint _elapsed_milliseconds; 
    
    bool _interaction_enabled;
    int _bordersize;
    double _boundary_width, _boundary_height;
    double _zoom_factor;
    
    Tile ***_tiles;   
    Tile *_currently_revealed_tiles[2];
    uint _cols, _rows;
    uint _num_pairs, _found_pairs;
    uint _num_clicked_tiles;    
    // A tile that is currently busy with turning over is moving. The user has the possibility to quickly 
    // click on two tiles, resulting in them both moving simultaniously. This is counted by num_moving_tiles:
    int _num_moving_tiles;
    
    // if two non-matching cards were revealed, they will be hidden the next click:
    bool _hide_tiles_next_click;
    // if two matching cards were revealed, they will be removed the next click:
    bool _remove_tiles_next_click;
    
};

#endif // MEMORYVIEW_H
