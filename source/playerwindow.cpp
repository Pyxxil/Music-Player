#include "includes/playerwindow.hpp"
#include "ui_playerwindow.h"

#include <QMediaMetaData>
#include <QMimeDatabase>
#include <QHBoxLayout>
#include <QTableView>
#include <QLabel>

// Taglib, at least on OSX, throws a couple of deprecated declaration warnings
// which are annoying to see, and interfere with -Werror. This might not be a
// good thing to do, but it solves this problem for now.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "mpegfile.h"
#include "attachedpictureframe.h"
#include "id3v2tag.h"
#include "id3v2extendedheader.h"
#include "mp4tag.h"
#include "mp4file.h"
#pragma GCC diagnostic pop

#include "includes/controls/durationcontrols.hpp"
#include "includes/controls/playercontrols.hpp"
#include "includes/controls/volumecontrols.hpp"
#include "includes/library/librarymodel.hpp"
#include "includes/menus/rightclickmenu.hpp"
#include "includes/library/libraryview.hpp"
#include "includes/trackinformation.hpp"
#include "includes/menus/menubar.hpp"

#include "includes/globals.hpp"

/*
 * TODO: Possible features
 * To add features list
 *	- Allow the stylesheet to be read from a file (.qss file) (This is sort of done already)
 *	- User adjustable previous song time limit (that is, how long into a song does pressing
 *	  the previous button restart the song)
 *	- Allow the user to remove/add headers and change the order of them
 *	    - These can be picked from whatever metadata can be grabbed
 *	- Saveability
 *	    - Save state
 *	    - Save library
 *	    - Save playlist's
 *	- Edit file metadata [*]
 *
 * TODO: Fixes
 *  - Fix up UI stuff (Most is done, just got to fix the gap between the cover art and the library)
 *  - Add other tags to columns in the library view (Partly done)
 *  - Fix the Playlist problems (playing next, previous crashes, change playlist set-up)
 *  - Set up a namespace that everything can connect to
 *      - Audio stuff
 *        - Global media player
 *      - UI stuff
 *        - Width & height percentages
 *  - Catch the window closing to do the following:
 *      - Saving
 *      - De-initialising items in the global namespace
 */

PlayerWindow::PlayerWindow(QWidget *parent)
        : QMainWindow(parent),
          ui(new Ui::PlayerWindow)
{
        ui->setupUi(this);

        // Project is defined in CMakeLists.txt
        setWindowTitle(Project);

        QFile styleSheet(":/StyleSheet.qss");
        styleSheet.open(QFile::ReadOnly | QFile::Text);
        QTextStream styles(&styleSheet);
        qApp->setStyleSheet(styles.readAll());

        Globals::init();

        playerControls = new PlayerControls(this);
        volumeControls = new VolumeControls(this, 150, 150);
        volumeControls->setContentsMargins(0, 0, 5, 0);
        durationControls = new DurationControls(this, 200);

        library = new LibraryModel;
        libraryView = new LibraryView(this, library);

        menu = new MenuBar(this);
        for (const auto &_menu : menu->getAllMenus()) {
                ui->menuBar->addMenu(_menu);
        }

        information = new TrackInformation(this, 200, 200);

        rightClickMenu = new RightClickMenu(this);

        coverArtLabel = new QLabel(this);
        coverArtLabel->setScaledContents(true);
        coverArtLabel->setBackgroundRole(QPalette::Base);
        coverArtLabel->setContentsMargins(10, 0, 0, 0);
        coverArtLabel->setMaximumSize(200, 200);
        coverArtLabel->setMinimumSize(200, 200);
        coverArtLabel->setAlignment(Qt::AlignCenter);

        setupConnections();
        setupUI();
}

PlayerWindow::~PlayerWindow()
{
        delete ui;
}

void PlayerWindow::nextSong()
{
        if (!Globals::getAudioInstance()->playlist()->isEmpty()) {
                Globals::getAudioInstance()->playlist()->next();
        }
}

/*
 * Go to the previous song, unless the time played into the current song is
 * less than 10 seconds.
 */
void PlayerWindow::previousSong()
{
        // TODO: Make the time to go to the previous song adjustable
        // TODO: What to do if the user has pressed go to previous and there aren't any songs
        // TODO: before it? Do we set position to 0, and pause the media? Or just reset the song?
        QMediaPlayer *player = Globals::getAudioInstance();
        if (player->position() < 10000 && player->playlist()->currentIndex() > 0) {
                player->playlist()->previous();
        } else {
                player->setPosition(0);
        }
}

void PlayerWindow::timeSeek(int time)
{
        Globals::getAudioInstance()->setPosition(time);
}

/*
 * In general, whenever the metadata updates, we want to update a few things:
 *      - The duration displayed by the duration controls
 *              - Either a new song is being played, and the duration is different than the previous
 *              - Or metadata has been edited to provide a new duration.
 *      - The window title, as this displays the current playing song
 *      - The information displayed below the cover art, and to the left of the controls.
 *      - We also want to update the cover art.
 */
void PlayerWindow::metaDataChanged()
{
        emit durationChanged(Globals::getAudioInstance()->duration());
        TagLib::FileRef song(
                QStringToTString(
                        Globals::getCurrentSong()
                                .toString()
                                .remove(0, 7))  // Remove the file:// prefix
                        .toCString());

        setWindowTitle(QString("%1 - %2")
                               .arg(TStringToQString(song.tag()->artist()))
                               .arg(TStringToQString(song.tag()->title())));
        emit trackInformationChanged(TStringToQString(song.tag()->artist()),
                                     TStringToQString(song.tag()->title()));

        loadCoverArt(song);
}

/*
 * Either begin playing the song, or continue playing the current song (the latter should be less
 * likely of an occurrence).
 */
void PlayerWindow::play()
{
        if (!Globals::getPlaylistInstance()->isEmpty()) {
                emit Globals::getAudioInstance()->play();
        }
}

/**
 * Play a song right now.
 */
void PlayerWindow::playNow()
{
        // TODO: Fix this.
        Globals::getPlaylistInstance()->insertMedia(0, library->get(libraryView->currentIndex().row()));
        Globals::getPlaylistInstance()->setCurrentIndex(0);
        emit Globals::getAudioInstance()->play();
}

/**
 * When the user clicks on the library, we want to show them a menu that they can use.
 * @param pos Where the user clicked.
 */
void PlayerWindow::customMenuRequested(QPoint pos)
{
        if (libraryView->indexAt(pos).isValid()) {
                library->indexMightBeUpdated(libraryView->indexAt(pos));
                emit rightClickMenu->display(libraryView->viewport()->mapToGlobal(pos),
                                             library->songAt(libraryView->indexAt(pos).row()));
        }
}

void PlayerWindow::updatePlaylist()
{
        // TODO: Figure out what to do here
}

void PlayerWindow::setupConnections()
{
        // TODO: Think about moving some of these into their respective classes
        QMediaPlayer *player = Globals::getAudioInstance();

        connect(playerControls, SIGNAL(play()),
                this, SLOT(play()));
        connect(playerControls, SIGNAL(pause()),
                player, SLOT(pause()));
        connect(playerControls, SIGNAL(next()),
                this, SLOT(nextSong()));
        connect(playerControls, SIGNAL(previous()),
                this, SLOT(previousSong()));
        connect(player, SIGNAL(stateChanged(QMediaPlayer::State)),
                playerControls, SLOT(setState(QMediaPlayer::State)));

        connect(volumeControls, SIGNAL(changeVolume(int)),
                player, SLOT(setVolume(int)));
        connect(volumeControls, SIGNAL(mute(bool)),
                player, SLOT(setMuted(bool)));

        connect(player, SIGNAL(volumeChanged(int)),
                volumeControls, SLOT(setVolume(int)));
        connect(player, SIGNAL(mutedChanged(bool)),
                volumeControls, SLOT(setMute(bool)));
        connect(player, SIGNAL(positionChanged(qint64)),
                durationControls, SLOT(positionChanged(qint64)));
        connect(player, SIGNAL(metaDataChanged()),
                this, SLOT(metaDataChanged()));
        connect(player, SIGNAL(stateChanged(QMediaPlayer::State)),
                menu, SLOT(playPauseChangeText(QMediaPlayer::State)));

        connect(durationControls, SIGNAL(seek(int)),
                this, SLOT(timeSeek(int)));

        connect(menu, SIGNAL(play()),
                player, SLOT(play()));
        connect(menu, SIGNAL(pause()),
                player, SLOT(pause()));
        connect(menu, SIGNAL(gotoNextSong()),
                this, SLOT(nextSong()));
        connect(menu, SIGNAL(gotoPreviousSong()),
                this, SLOT(previousSong()));
        connect(menu, SIGNAL(updateLibrary()),
                library, SLOT(openDirectory()));

        connect(library, SIGNAL(libraryUpdated()),
                this, SLOT(updatePlaylist()));

        connect(this, SIGNAL(trackInformationChanged(QString, QString)),
                information, SLOT(updateLabels(QString, QString)));
        connect(this, SIGNAL(durationChanged(qint64)),
                durationControls, SLOT(songChanged(qint64)));

        connect(libraryView, SIGNAL(customContextMenuRequested(QPoint)),
                this, SLOT(customMenuRequested(QPoint)));
        connect(libraryView->horizontalHeader(), SIGNAL(sectionClicked(int)),
                library, SLOT(sortByColumn(int)));
        connect(libraryView, SIGNAL(doubleClicked(const QModelIndex &)),
                this, SLOT(playNow()));

        connect(rightClickMenu, SIGNAL(playThisNow()),
                this, SLOT(playNow()));
        connect(rightClickMenu, SIGNAL(updateLibrary()),
                library, SLOT(updateMetadata()));
}

void PlayerWindow::setupUI()
{
        QHBoxLayout *centralPlayerControlsLayout = new QHBoxLayout;
        centralPlayerControlsLayout->addStretch(1);
        centralPlayerControlsLayout->addSpacing(1);
        centralPlayerControlsLayout->addWidget(playerControls);
        centralPlayerControlsLayout->addStretch(1);
        centralPlayerControlsLayout->addSpacing(1);
        centralPlayerControlsLayout->setContentsMargins(0, 0, 0, 0);

        QVBoxLayout *playerDurationControlsLayout = new QVBoxLayout;
        playerDurationControlsLayout->addLayout(centralPlayerControlsLayout);
        playerDurationControlsLayout->addWidget(durationControls);
        playerDurationControlsLayout->setContentsMargins(0, 0, 0, 0);

        QHBoxLayout *controlLayout = new QHBoxLayout;
        controlLayout->addWidget(information);
        controlLayout->addLayout(playerDurationControlsLayout);
        controlLayout->addWidget(volumeControls);
        controlLayout->setContentsMargins(0, 0, 0, 0);

        QWidget *controlsWidget = new QWidget(this);
        controlsWidget->setLayout(controlLayout);
        controlsWidget->setMaximumHeight(120);
        controlsWidget->setContentsMargins(0, 0, 0, 0);
        controlsWidget->setStyleSheet("border: none;");

        QVBoxLayout *coverArtArea = new QVBoxLayout;
        coverArtArea->setContentsMargins(0, 0, 0, 0);
        coverArtArea->addStretch(1);
        coverArtArea->addSpacing(1);
        coverArtArea->addWidget(coverArtLabel);

        QHBoxLayout *uiLayout = new QHBoxLayout;
        uiLayout->addLayout(coverArtArea);
        uiLayout->addWidget(libraryView, 1);
        uiLayout->setContentsMargins(0, 0, 0, 0);

        QVBoxLayout *endLayout = new QVBoxLayout;
        endLayout->addLayout(uiLayout);
        endLayout->addWidget(controlsWidget);
        endLayout->setContentsMargins(0, 0, 0, 0);

        ui->centralWidget->setLayout(endLayout);
}

/**
 * Load the cover art of the currently playing song, and put it on the display.
 * @param song The currently playing song.
 */
void PlayerWindow::loadCoverArt(TagLib::FileRef &song)
{
        QMimeDatabase db;
        QMimeType codec = db.mimeTypeForFile(song.file()->name());

        bool coverArtNotFound = true;

        if (codec.name() == "audio/mp4") {
                TagLib::MP4::File mp4(song.file()->name());
                if (mp4.tag() && mp4.tag()->itemListMap().contains("covr")) {
                        TagLib::MP4::CoverArtList coverArtList =
                                mp4.tag()->itemListMap()["covr"].toCoverArtList();

                        if (!coverArtList.isEmpty()) {
                                TagLib::MP4::CoverArt coverArt = coverArtList.front();
                                image.loadFromData((const uchar *) coverArt.data().data(),
                                                   coverArt.data().size());
                                coverArtNotFound = false;
                        }
                }
        } else if (codec.name() == "audio/mpeg") {
                TagLib::MPEG::File file(song.file()->name());
                TagLib::ID3v2::FrameList frameList = file.ID3v2Tag()->frameList("APIC");

                if (!frameList.isEmpty()) {
                        TagLib::ID3v2::AttachedPictureFrame
                                *coverImg = static_cast<TagLib::ID3v2::AttachedPictureFrame *>(frameList.front());

                        image.loadFromData((const uchar *) coverImg->picture().data(),
                                           coverImg->picture().size());
                        coverArtNotFound = false;
                }
        }

        if (coverArtNotFound) {
                image.load(":/CoverArtUnavailable.png");
        }

        coverArtLabel->setPixmap(QPixmap::fromImage(image));
}

void PlayerWindow::closeEvent(QCloseEvent *event)
{
        Globals::deInit();
        QMainWindow::closeEvent(event);
}
