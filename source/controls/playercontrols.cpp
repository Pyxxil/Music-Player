#include "includes/controls/playercontrols.hpp"

#include <QToolButton>
#include <QBoxLayout>

#include "includes/astoria.hpp"

static constexpr int iconWidth = 20;
static constexpr int iconHeight = 20;

PlayerControls::PlayerControls(QWidget *parent)
        : QWidget(parent),
          playerState(Astoria::getAudioInstance()->state())
{
        // Move it closer to the library above it, and the duration control below it
        setContentsMargins(0, -20, 0, -10);
        setStyleSheet("border: none;");

        playPauseIcon.addPixmap(QPixmap(":/assets/icons/Play.png"), QIcon::Normal, QIcon::On);
        playPauseIcon.addPixmap(QPixmap(":/assets/icons/Pause.png"), QIcon::Normal, QIcon::Off);

        shuffleIcon.addPixmap(QPixmap(":/assets/icons/ShuffleOff.png"), QIcon::Normal, QIcon::Off);
        shuffleIcon.addPixmap(QPixmap(":/assets/icons/ShuffleOn.png"), QIcon::Normal, QIcon::On);

        repeatIcon.addPixmap(QPixmap(":/assets/icons/RepeatOff.png"), QIcon::Normal, QIcon::Off);
        repeatIcon.addPixmap(QPixmap(":/assets/icons/RepeatOn.png"), QIcon::Normal, QIcon::On);

        nextIcon.addPixmap(QPixmap(":/assets/icons/Next.png"), QIcon::Normal, QIcon::On);
        nextIcon.addPixmap(QPixmap(":/assets/icons/NextActive.png"), QIcon::Active, QIcon::On);

        previousIcon.addPixmap(QPixmap(":/assets/icons/Previous.png"), QIcon::Normal, QIcon::On);
        previousIcon.addPixmap(QPixmap(":/assets/icons/PreviousActive.png"), QIcon::Active, QIcon::On);

        playPauseButton = new QToolButton(this);
        playPauseButton->setIcon(playPauseIcon);
        playPauseButton->setIconSize(QSize(static_cast<int>(iconWidth * 1.5),
                                           static_cast<int>(iconHeight * 1.5)));
        playPauseButton->setCheckable(true);
        playPauseButton->setChecked(true);
        playPauseButton->setShortcut(Qt::Key_Space);
        connect(playPauseButton, SIGNAL(clicked()),
                this, SLOT(playPauseButtonClicked()));

        nextButton = new QToolButton(this);
        nextButton->setIcon(nextIcon);
        nextButton->setIconSize(QSize(iconWidth, iconHeight));
        connect(nextButton, SIGNAL(clicked()),
                this, SLOT(nextButtonClicked()));

        previousButton = new QToolButton(this);
        previousButton->setIcon(previousIcon);
        previousButton->setIconSize(QSize(iconWidth, iconHeight));
        connect(previousButton, SIGNAL(clicked()),
                this, SLOT(previousButtonClicked()));

        shuffleButton = new QToolButton(this);
        shuffleButton->setIcon(shuffleIcon);
        shuffleButton->setIconSize(QSize(iconWidth, iconHeight));
        shuffleButton->setCheckable(true);

        repeatButton = new QToolButton(this);
        repeatButton->setIcon(repeatIcon);
        repeatButton->setIconSize(QSize(iconWidth, iconHeight));
        repeatButton->setCheckable(true);

        setLayout(new QHBoxLayout);
        layout()->addWidget(shuffleButton);
        layout()->addWidget(previousButton);
        layout()->addWidget(playPauseButton);
        layout()->addWidget(nextButton);
        layout()->addWidget(repeatButton);

        connect(this, SIGNAL(pause()),
                Astoria::getAudioInstance(), SLOT(pause()));
        connect(Astoria::getAudioInstance(), SIGNAL(stateChanged(QMediaPlayer::State)),
                this, SLOT(setState(QMediaPlayer::State)));
}

void PlayerControls::setState(QMediaPlayer::State state)
{
        if (state != playerState) {
                playerState = state;

                switch (state) {
                case QMediaPlayer::PlayingState:
                        playPauseButton->setChecked(false);
                        break;
                case QMediaPlayer::PausedState:
                case QMediaPlayer::StoppedState:
                        playPauseButton->setChecked(true);
                        break;
                }
        }
}

void PlayerControls::playPauseButtonClicked()
{
        switch (playerState) {
        case QMediaPlayer::StoppedState:
        case QMediaPlayer::PausedState:emit play();
                break;
        case QMediaPlayer::PlayingState:emit pause();
                break;
        }
}

void PlayerControls::nextButtonClicked()
{
        emit next();
}

void PlayerControls::previousButtonClicked()
{
        emit previous();
}
