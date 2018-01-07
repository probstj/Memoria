#include "memory.h"

Memory::Memory() : _the_AI(NULL), _verbose(!true)
{
    setWindowTitle(QCoreApplication::applicationName());
    
    // initialize pseudo random generator:
    QTime midnight(0, 0, 0);
    srand(uint(midnight.msecsTo(QTime::currentTime())));

    // load font ressources:
    loadFont();
    
    // load and set the central widget:
    QString backsideImageFileName(QApplication::applicationDirPath() + "/backside.png");
    if (!QFile::exists(backsideImageFileName))
        // if file does not exist, use the file in the ressource:
        backsideImageFileName = ":/backside.png";
    _the_view = new MemoryView(4, backsideImageFileName, 2.5, this);
    setCentralWidget(_the_view);
    
    // save settings in INI file:
    QSettings::setDefaultFormat(QSettings::IniFormat);
    readSettings();
    
    // connect signals:
    connect(_the_view, SIGNAL(matchFound()),
            this,  SLOT(matchFound()));
    connect(_the_view, SIGNAL(matchFailed()),
            this,  SLOT(matchFailed()));
    connect(_the_view, SIGNAL(tileRevealed(uint, uint, uint)),
            this,  SLOT(tileRevealed(uint, uint, uint)));
    connect(_the_view, SIGNAL(boardReady()),
            this, SLOT(boardReady()));
    
    // add some menu items:
    QMenu *filemenu = menuBar()->addMenu(tr("&File"));
    
    QAction* a = new QAction(this);
    a->setText(tr("&New game"));
    a->setShortcut(QKeySequence::New);
    connect(a, SIGNAL(triggered()), SLOT(startNewGame()) );
    filemenu->addAction(a);

    a = new QAction(this);
    a->setText(tr("&Change image folder"));
    connect(a, SIGNAL(triggered()), SLOT(changeImageFolder()) );
    filemenu->addAction(a);
    
    filemenu->addSeparator();
    a = new QAction(this);
    a->setText(tr("&Quit")); 
    connect(a, SIGNAL(triggered()), SLOT(close()) );
    filemenu->addAction(a);
    
    QSettings settings;
    // write default value to ini:
    if (!settings.contains("path_to_images"))
        settings.setValue("path_to_images", tr("./images"));
    QString path = settings.value("path_to_images").toString();
    QDir dir(QApplication::applicationDirPath());
    dir.setPath(path); // this can be an absolute path or relative path to applicationDirPath
    path = dir.absolutePath();
    
    // load the images:
    setImagePath(path);
    printf("found %i image files in %s.\n", _image_file_names.count(), path.toStdString().c_str());
    bool cancelled = false;
    while (_image_file_names.count() < 2 && !cancelled) {
        QMessageBox::warning(this, QCoreApplication::applicationName(),
                             tr("No or not enough images were found in the folder %1.\n\n").arg(path) +
                             tr("In the following, you can select another folder with images."));
        path = QFileDialog::getExistingDirectory(this, tr("Select image folder"), path);
        if (path.isEmpty()) {
            // user cancelled:
            cancelled = true;
        }
        else {
            // try again with new path:
            setImagePath(path);
            printf("found %i image files in %s.\n", _image_file_names.count(), path.toStdString().c_str());
            if (_image_file_names.count() >= 2) {
                // save the new path to config file:
                settings.setValue("path_to_images", path);
            }
        }
    }
    
    // create dialog for new game options:
    _new_dialog = new NewGameDialog(_image_file_names.count(), this);

    _player1_high_score = settings.value("High_score", 0.0).toInt();
}

void Memory::setImagePath(const QString path)
{
    _image_path = QDir(path);
    QStringList filenamefilter, found_files;
    // get list of supported image formats:
    QList<QByteArray> sifs(QImageReader::supportedImageFormats());
    for (int i = 0; i < sifs.count(); i++) {
        filenamefilter << "*." + sifs.at(i);
        // this can be uncommented to print supported files:
        //printf("%s\n", filenamefilter.at(i).toStdString().c_str());
    }
    found_files = _image_path.entryList(filenamefilter, QDir::Files | QDir::Readable);
    _image_file_names.clear();
    for (int i = 0; i < found_files.count(); i++)
        _image_file_names << "img:" + found_files.at(i);
    QDir::setSearchPaths("img", QStringList(_image_path.absolutePath()));
}

bool Memory::startNewGame()
{
    if (_image_file_names.count() < 2) {
        // must have some images in order to start a game
        return false;
    }
    // pause timer if it was running (so we can continue if startNewGame dialog is cancelled):
    bool timer_was_running = _the_view->isTimerRunning();
    _the_view->stopTimer();
    if (_new_dialog->exec())
    {
        _the_view->hideStatusText();
        
        int cols, rows;
        _the_view->calculate_best_distribution(cols, rows, _new_dialog->getNumberOfPairs() * 2);
        
        // delete previous AI:
        if (_the_AI) {
            delete _the_AI;
            _the_AI = NULL;
        }
        
        _opponent = _new_dialog->getOpponent();
        if (_opponent == COMPUTER_OPPONENT) {
            _the_AI = new MemoryAI(cols, rows, 
                                _new_dialog->getNumberOfPairs() * 2, 
                                _new_dialog->getDifficultyLevel(),
                                this);
            _the_AI->setVerbosity(_verbose);
            connect(_the_view, SIGNAL(tileRevealed(uint,uint,uint)), _the_AI, SLOT(revealedTile(uint,uint,uint)));
            connect(_the_AI, SIGNAL(submitGuess(uint,uint)), _the_view, SLOT(revealTile(uint,uint)));
            if (_new_dialog->getStartingPlayer() == RANDOM_STARTS) {
                _current_player = get_random_in_interval(0, 1) ? PLAYER1 : COMPUTER;
            }
            else if (_new_dialog->getStartingPlayer() == COMPUTER_STARTS)
                _current_player = COMPUTER;
            else
                _current_player = PLAYER1;
        }
        else {
            _current_player = PLAYER1;
        }
        _current_player_moves = 0;
        for (int i = 0; i < 3; ++i) {
            _num_found_pairs[i] = 0;
            _num_fails[i] = 0;
        }
        
        _the_view->enableUserInteraction(_current_player != COMPUTER);
        if (!_the_view->set_images(_new_dialog->getNumberOfPairs(), cols, rows, _image_file_names)) {
            QMessageBox::warning(this, QCoreApplication::applicationName(), tr("load failed"));
            return false;
        }
        if (_opponent == NO_OPPONENT) {
            // show found pairs and fails of player, but with empty name,
            // because it is the only player:
            _the_view->setupPlayers(QStringList() << "");
            // Show time, but don't start timer yet:
            _the_view->showPlayingTime(false);
            // Timer will start if all images are loaded:
            connect(_the_view, SIGNAL(imagesLoaded()),
                this, SLOT(imagesLoaded()));
        }
        else if (_opponent == HUMAN_OPPONENT) 
            _the_view->setupPlayers(QStringList() << tr("Player 1") << tr("Player 2"));
        else if (_opponent == COMPUTER_OPPONENT) 
            _the_view->setupPlayers(QStringList() << tr("You") << tr("Computer"));
        return true;
    }
    // dialog was cancelled
    if (timer_was_running)
        _the_view->continueTimer();
    return false;
}

void Memory::changeImageFolder() {
    // backup previous path in case of invalid new path:
    QString path = _image_path.absolutePath(), previous_path = path;
    _image_file_names.clear();
    QSettings settings;

    // load the images:
    bool cancelled = false;
    while (_image_file_names.count() < 2 && !cancelled) {
        path = QFileDialog::getExistingDirectory(this, tr("Select image folder"), path);
        if (path.isEmpty()) {
            // user cancelled:
            cancelled = true;
        }
        else {
            // try new path:
            setImagePath(path);
            printf("found %i image files in %s.\n", _image_file_names.count(), path.toStdString().c_str());
            if (_image_file_names.count() >= 2) {
                // save the new path to config file:
                settings.setValue("path_to_images", path);
            }
            else {
                QMessageBox::warning(this, QCoreApplication::applicationName(),
                     tr("No or not enough images were found in the folder %1.\n\n").arg(path) +
                     tr("Please select another folder with images."));
            }
        }
    }
    if (cancelled) {
        // user canceled, revert to previous path:
        path = previous_path;
        setImagePath(path);
        printf("found %i image files in %s.\n", _image_file_names.count(), path.toStdString().c_str());
    }
    else {
        QMessageBox::information(this, QCoreApplication::applicationName(),
            tr("Found %1 images in the new folder.\n").arg(_image_file_names.count()) +
            tr("The new images will be used the next time you start a new game."));
    }

    // update dialog for new game options:
    _new_dialog->setMaxPairs(_image_file_names.count());
}

void Memory::matchFound()
{
    if (_verbose)
        printf("Match found!\n");
    if (_the_view->is_game_over())
        _the_view->showStatusText("Klicken, dann ist es geschafft!"); // TODO tr
    else if (_current_player == COMPUTER)
        _the_view->showStatusText("Klicken, dann ist der Rechner nochmal dran!");
    else if (_current_player == PLAYER2)
        _the_view->showStatusText("Klicken, dann ist Spieler 2 nochmal dran!");
    else if (_opponent == HUMAN_OPPONENT)
        _the_view->showStatusText("Klicken, dann ist Spieler 1 nochmal dran!");
    else if (_opponent == NO_OPPONENT)
        _the_view->showStatusText("Klicken, dann geht's weiter!");
    else 
        _the_view->showStatusText("Klicken, dann bist du nochmal dran!");
    _num_found_pairs[_current_player]++;
    submitCurrentScore();
    // the current player has another turn:
    _current_player_moves = 0;
}                        

void Memory::matchFailed()
{
    if (_verbose)
        printf("Match failed.\n");
    _num_fails[_current_player]++;
    submitCurrentScore();
    // next player's turn:
    if (_opponent == COMPUTER_OPPONENT)
        _current_player = _current_player == PLAYER1 ? COMPUTER : PLAYER1;
    else if (_opponent == HUMAN_OPPONENT)
        _current_player = _current_player == PLAYER1 ? PLAYER2 : PLAYER1;
    if (_current_player == COMPUTER)
        _the_view->showStatusText("Klicken, dann ist der Rechner dran!"); // TODO tr
    else if (_current_player == PLAYER2)
        _the_view->showStatusText("Klicken, dann ist Spieler 2 dran!");
    else if (_opponent == HUMAN_OPPONENT)
        _the_view->showStatusText("Klicken, dann ist Spieler 1 dran!");
    else if (_opponent == NO_OPPONENT)
        _the_view->showStatusText("Klicken, dann geht's weiter!");
    else    
        _the_view->showStatusText("Klicken, dann bist du an der Reihe!");
    _current_player_moves = 0;
    _the_view->enableUserInteraction(_current_player != COMPUTER);
}

void Memory::tileRevealed(uint id, uint col, uint row)
{
    if (_verbose)
        printf("Tile at position <%i, %i> has been revealed. ID is: %i\n", col, row, id);
    _current_player_moves++;
}

void Memory::boardReady()
{
    if (!_the_view->is_game_over()) {
        if (_verbose)
            printf("board ready, waiting for input from %s\n", 
               _current_player == COMPUTER ? "PC" : _current_player == PLAYER1 ? "player 1" : "player 2");
        if (_current_player == COMPUTER) {
            _the_view->showStatusText("Der Rechner macht seinen Zug..."); // TODO tr
            if (_current_player_moves == 0) 
                _the_AI->firstGuess();
            else
                _the_AI->secondGuess();
        }
        else if (_current_player == PLAYER2) 
            _the_view->showStatusText("Spieler 2 ist dran.");
        else if (_opponent == HUMAN_OPPONENT)
            _the_view->showStatusText("Spieler 1 ist dran."); 
        else if (_opponent == COMPUTER_OPPONENT)
            _the_view->showStatusText("Du bist dran.");
        else
            // no need to show text if single player:
            _the_view->hideStatusText();
    }
    else {
        showFinalScore();
        if (!startNewGame())
            QApplication::exit();
    }
}

void Memory::imagesLoaded() {
    if (_opponent == NO_OPPONENT && !_the_view->isTimerRunning()) {
        _the_view->startTimer();
    }
    // don't need connection anymore:
    disconnect(_the_view, SIGNAL(imagesLoaded()),
        this, SLOT(imagesLoaded()));
}

void Memory::closeEvent(QCloseEvent* event)
{
    //if (userReallyWantsToQuit()) {
        writeSettings();
        event->accept();
    //} else {
    //    event->ignore();
    //}
}


void Memory::writeSettings()
{
    QSettings settings;
    
    settings.beginGroup("MainWindow");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.endGroup();
}

void Memory::readSettings()
{
    QSettings settings;
    
    settings.beginGroup("MainWindow");
    resize(settings.value("size", QSize(800, 600)).toSize());
    move(settings.value("pos", QPoint(100, 100)).toPoint());
    settings.endGroup();
}

void Memory::loadFont()
{
    QStringList list;
    list << "DejaVuSans.ttf" << "DejaVuSansMono.ttf" << "DejaVuSans-Bold.ttf" << "DejaVuSansMono-Bold.ttf";
    int fontID(-1);
    QString fontWarning("Could not open font file '%1' from ressources.");
    for (QStringList::const_iterator constIterator = list.constBegin(); constIterator != list.constEnd(); ++constIterator) {
        QFile res(":/" + *constIterator);
        if (res.open(QIODevice::ReadOnly) == false) {
            QMessageBox::warning(this, QCoreApplication::applicationName(), fontWarning.arg(":/" + *constIterator));
        } else {
            fontID = QFontDatabase::addApplicationFontFromData(res.readAll());
            if (fontID == -1) {
                QMessageBox::warning(this, QCoreApplication::applicationName(), fontWarning.arg(":/" + *constIterator));
            }
        }
    }
}

void Memory::submitCurrentScore() const
{
    int pairs[2] = {0, 0}; 
    int errors[2] = {0, 0};
    if (_opponent == NO_OPPONENT) {
        pairs[0] = _num_found_pairs[PLAYER1];
        errors[0] = _num_fails[PLAYER1];
    }
    else if (_opponent == HUMAN_OPPONENT) {
        pairs[0] = _num_found_pairs[PLAYER1];
        errors[0] = _num_fails[PLAYER1];
        pairs[1] = _num_found_pairs[PLAYER2];
        errors[1] = _num_fails[PLAYER2];
    }
    else if (_opponent == COMPUTER_OPPONENT) {
        pairs[0] = _num_found_pairs[PLAYER1];
        errors[0] = _num_fails[PLAYER1];
        pairs[1] = _num_found_pairs[COMPUTER];
        errors[1] = _num_fails[COMPUTER];
    }
    _the_view->updateCurrentScore(pairs, errors);
}


void Memory::showFinalScore()
{
    int score = 0;

    if (_verbose) {
        printf("computer found %i pairs, made %i mistakes\n", _num_found_pairs[COMPUTER], _num_fails[COMPUTER]);
        printf("player1 found %i pairs, made %i mistakes\n", _num_found_pairs[PLAYER1], _num_fails[PLAYER1]);
        printf("player2 found %i pairs, made %i mistakes\n", _num_found_pairs[PLAYER2], _num_fails[PLAYER2]);
    }
    QString winText;
    if (_opponent == HUMAN_OPPONENT) {
        if (_num_found_pairs[PLAYER1] > _num_found_pairs[PLAYER2])
            winText = tr("Player 1") % (" ") % tr("wins!");
            else if (_num_found_pairs[PLAYER1] < _num_found_pairs[PLAYER2])
                winText = tr("Player 2") % (" ") % tr("wins!");
            else
                winText = tr("tie!");
    }
    else if (_opponent == COMPUTER_OPPONENT) {
        if (_num_found_pairs[PLAYER1] > _num_found_pairs[COMPUTER_OPPONENT])
            winText = tr("You win!");
        else if (_num_found_pairs[PLAYER1] < _num_found_pairs[COMPUTER_OPPONENT])
            winText = tr("The computer") % (" ") % tr("wins!");
        else
            winText = tr("tie!");
    }
    else if (_opponent == NO_OPPONENT) {
        score = getSinglePlayerPoints();
        winText = tr("Your score: ") + QString::number(score);
    }
    _the_view->showStatusText(winText);

    QString richText;
    QTextStream tstream(&richText);
    if (_opponent == NO_OPPONENT) {
        QTime time(_the_view->getPlayingTime());
        tstream << "<!DOCTYPE html><html>"
                   "<head><style>th, td {vertical-align: middle}</style></head>"
                   "<body>"
                   "  <br><b><big><center>" << winText << "</center></big></b>"
                   "  <br><center>" << tr("Previous high score: %1").arg(_player1_high_score) << "</center> <br>"
                   // place a table inside a table so we have a border (1st table's cellpadding)
                   // around the actual visible table (otherwise the QMessageBox will have the
                   // size of the visible table, leaving no space right of the table):
                   "  <table border=\"0\" width=\"100%\" cellspacing=\"0\" cellpadding=\"10\">"
                   "    <tr><td>"
                   "      <table border=\"1\" width=\"100%\" cellspacing=\"0\" cellpadding=\"8\">"
                   "        <tr> "
                   "          <th width = 50%>" << tr("Found pairs") << "</th>"
                   "          <td align=\"center\">" << _num_found_pairs[PLAYER1] << "</td>"
                   "        </tr>"
                   "        <tr> "
                   "          <th>" << tr("Mistakes") << "</th>"
                   "          <td align=\"center\">" << _num_fails[PLAYER1] << "</td>"
                   "        </tr>"
                   "        <tr> "
                   "          <th>" << tr("Needed time") << "</th>"
                   "          <td align=\"center\">";
        if (time.hour() > 0)
            tstream << time.toString(tr("h 'hrs.' m 'min.' s 'sec.'")) << "</td>";
        else
            tstream << time.toString(tr("m 'min.' s 'sec.'")) << "</td>";
        tstream << "        </tr>"
                   "      </table> <br><br>";

        tstream << "      <table border=\"1\" width=\"100%\" cellspacing=\"0\" cellpadding=\"8\">"
                   "        <caption>" << tr("Score calculation:") << "</caption>"
                   "        <tr> "
                   "          <td width = 50% align=\"center\">" << tr("Number of<br>cards x 20") << "</td> "
                   "          <td align=\"center\">" << _new_dialog->getNumberOfPairs() * 2 * 20 << "</td>"
                   "        </tr>"
                   "        <tr> "
                   "          <td align=\"center\">" << tr("less<br>seconds elapsed") << "</td>"
                   "          <td align=\"center\">" << time.secsTo(QTime()) << "</td>"
                   "        </tr>"
                   "        <tr> "
                   "          <td align=\"center\">" << tr("less<br>mistakes x 10") << "</td>"
                   "          <td align=\"center\">" << (-10 * (int)_num_fails[PLAYER1]) << "</td>"
                   "        </tr>"
                   "      </table>"
                   "    </td>"
                   // another td cell to the right for more space on the right:
                   "    <td></td></tr>"
                   "  </table> <br>"
                   "</body></html>";

        // update high score:
        if (score > _player1_high_score) {
            _player1_high_score = score;
            QSettings settings;
            settings.setValue("High_score", _player1_high_score);
            settings.sync();
        }
    } else {
        QString player1name, player2name;
        PLAYER secondplayer;
        if (_opponent == COMPUTER_OPPONENT) {
            player1name = tr("You");
            player2name = tr("The computer");
            secondplayer = COMPUTER;
        } else {
            player1name = tr("Player 1");
            player2name = tr("Player 2");
            secondplayer = PLAYER2;
        }
        tstream << "<!DOCTYPE html><html>"
                   "<head><style>th, td {vertical-align: middle}</style></head>"
                   "<body>"
                   "  <br><b><big><center>" << winText << "</center></big></b> <br>"
                   // place a table inside a table so we have a border (1st table's cellpadding)
                   // around the actual visible table (otherwise the QMessageBox will have the
                   // size of the visible table, leaving no space right of the table):
                   "  <table border=\"0\" width=\"100%\" cellspacing=\"0\" cellpadding=\"10\">"
                   "    <tr><td>"
                   "      <table border=\"1\" width=\"100%\" cellspacing=\"0\" cellpadding=\"8\">"
                   "        <tr> "
                   "          <th></th>"
                   "          <th>" << tr("Found<br>pairs") << "</th>"
                   "          <th>" << tr("Mistakes") << "</th>"
                   "        </tr>"
                   "        <tr> "
                   "          <th>" << player1name << "</th>"
                   "          <td align=\"center\">" << _num_found_pairs[PLAYER1] << "</td>"
                   "          <td align=\"center\">" << _num_fails[PLAYER1] << "</td>"
                   "        </tr>"
                   "        <tr> "
                   "          <th>" << player2name << "</th>"
                   "          <td align=\"center\">" << _num_found_pairs[secondplayer] << "</td>"
                   "          <td align=\"center\">" << _num_fails[secondplayer] << "</td>"
                   "        </tr>"
                   "      </table>"
                   "    </td>"
                   // another td cell to the right for more space on the right:
                   "    <td></td></tr>"
                   "  </table> <br>"
                   "</body></html>";
    }
    QMessageBox::information(this, QCoreApplication::applicationName(), richText);
}

int Memory::getSinglePlayerPoints() const
{
    // TODO: make this configurable via QSettings:
    return _new_dialog->getNumberOfPairs() * 2 * 20 - QTime().secsTo(_the_view->getPlayingTime()) - _num_fails[PLAYER1] * 10;
}

// necessary for Qt's meta objectc compiler, e.g. for signal-slot-system:
//#include "memory.moc"
