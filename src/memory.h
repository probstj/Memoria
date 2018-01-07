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

#ifndef MEMORY_H
#define MEMORY_H

#include <QMainWindow>
#include <QApplication>
#include <QSettings>
#include <QMenuBar>
#include <QDir>
#include <QTime>
#include <QImageReader>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QFontDatabase>
#include "memoryview.h"
#include "newgamedialog.h"
#include "MemoryAI.h"

class Memory : public QMainWindow
{
    // necessary for Qt's meta objectc compiler, e.g. for signal-slot-system:
    Q_OBJECT
    
public:
    Memory();
    void setImagePath(const QString path);
    
public slots:
    bool startNewGame();
    void changeImageFolder();
    void matchFound();
    void matchFailed(); // TODO: differ between unlucky fail and fail if correct cards should have been known.
    
    // To keep track of revealed cards, e.g. for A.I. opponent:
    void tileRevealed(uint id, uint col, uint row);
    
    void boardReady();
    void imagesLoaded();
    
protected:
    virtual void closeEvent(QCloseEvent* event);
    
private:
    enum PLAYER
    {
        PLAYER1 = 0,
        PLAYER2 = 1,
        COMPUTER = 2
    };
    
    // saves and reads the window's size & position to/from an INI file:
    void writeSettings();
    void readSettings();

    // loads the used fonts from the ressource file:
    void loadFont();
    
    void submitCurrentScore() const;
    void showFinalScore();
    int getSinglePlayerPoints() const;
    
    MemoryView *_the_view;
    MemoryAI *_the_AI;
    QDir _image_path;
    QStringList _image_file_names;
    NewGameDialog *_new_dialog;
    PLAYER _current_player;
    OPPONENT _opponent;
    // the number of moves the current player has done already:
    uint _current_player_moves;
    // the number of pairs each player have found:
    uint _num_found_pairs[3];
    // the number of mistakes each player made:
    uint _num_fails[3];
    // the high score of the single player game:
    int _player1_high_score;
    // if this is true, prints out what is happening to stdout (set in constructor):
    bool _verbose;
    
};

#endif // MEMORY_H
