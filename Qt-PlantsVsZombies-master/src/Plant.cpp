#include <QtMultimedia>
#include "Plant.h"
#include "ImageManager.h"
#include "GameScene.h"
#include "GameLevelData.h"
#include "MouseEventPixmapItem.h"
#include "Timer.h"
#include "Animate.h"

Plant::Plant()
        : hp(300),
          pKind(1), bKind(0),
          beAttackedPointL(20), beAttackedPointR(20),
          zIndex(0),
          canEat(true), canSelect(true), night(false),
          coolTime(7.5), stature(0), sleep(0), scene(nullptr)
{}

double Plant::getDX() const
{
    return -0.5 * width;
}

double Plant::getDY(int x, int y) const
{
    return scene->getPlant(x, y).contains(0) ? -21 : -15;
}

bool Plant::canGrow(int x, int y) const
{
    if (x < 1 || x > 9 || y < 1 || y > 5)
        return false;
    if (scene->isCrater(x, y) || scene->isTombstone(x, y))
        return false;
    int groundType = scene->getGameLevelData()->LF[y];
    QMap<int, PlantInstance *> plants = scene->getPlant(x, y);
    if (groundType == 1)
        return !plants.contains(1);
    return plants.contains(0) && !plants.contains(1);
}

void Plant::update()
{
    QPixmap pic = gImageCache->load(staticGif);
    width = pic.width();
    height = pic.height();
}

PlantInstance::PlantInstance(const Plant *plant)
    : plantProtoType(plant),
      picture(new MoviePixmapItem),
      attackMusic(new QMediaPlayer(picture)),
      dieMusic(new QMediaPlayer(picture))
{
    uuid = QUuid::createUuid();
    hp = plantProtoType->hp;
    canTrigger = true;
    canVaulted = true;
}

PlantInstance::~PlantInstance()
{
    picture->deleteLater();
}

void PlantInstance::birth(int c, int r)
{
    Coordinate &coordinate = plantProtoType->scene->getCoordinate();
    double x = coordinate.getX(c) + plantProtoType->getDX(), y = coordinate.getY(r) + plantProtoType->getDY(c, r) - plantProtoType->height;
    col = c, row = r;
    attackedLX = x + plantProtoType->beAttackedPointL;
    attackedRX = x + plantProtoType->beAttackedPointR;
    picture->setMovie(plantProtoType->normalGif);
    picture->setPos(x, y);
    picture->setZValue(plantProtoType->zIndex + 3 * r);//???????????????
    shadowPNG = new QGraphicsPixmapItem(gImageCache->load("interface/plantShadow.png"));//??????????????????
    shadowPNG->setPos(plantProtoType->width * 0.5 - 48, plantProtoType->height - 22);
    shadowPNG->setFlag(QGraphicsItem::ItemStacksBehindParent);
    shadowPNG->setParentItem(picture);
    picture->start();
    plantProtoType->scene->addToGame(picture);
    initTrigger();
}

void PlantInstance::initTrigger()
{
    Trigger *trigger = new Trigger(this, attackedLX, 880, 0, 0);
    triggers.insert(row, QList<Trigger *>{ trigger } );
    plantProtoType->scene->addTrigger(row, trigger);//??????????????????????????????
}

bool PlantInstance::contains(const QPointF &pos)
{
    return picture->contains(pos);
}

void PlantInstance::triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger)//??????
{
    if (zombieInstance->altitude > 0) {
        canTrigger = false;
        QSharedPointer<std::function<void(QUuid)> > triggerCheck(new std::function<void(QUuid)>);
        *triggerCheck = [this, triggerCheck] (QUuid zombieUuid) {
            (new Timer(picture, 1400, [this, zombieUuid, triggerCheck] {
                ZombieInstance *zombie = this->plantProtoType->scene->getZombie(zombieUuid);
                if (zombie) {
                    for (auto i: triggers[zombie->row]) {
                        if (zombie->hp > 0 && i->from <= zombie->ZX && i->to >= zombie->ZX && zombie->altitude > 0) {
                            normalAttack(zombie);
                            (*triggerCheck)(zombie->uuid);
                            return;
                        }
                    }
                }
                canTrigger = true;//?????????????????????
            }))->start();
        };
        normalAttack(zombieInstance);
        (*triggerCheck)(zombieInstance->uuid);
    }
}

void PlantInstance::normalAttack(ZombieInstance *zombieInstance)
{
    qDebug() << plantProtoType->cName << uuid << "Attack" << zombieInstance->zombieProtoType->cName << zombieInstance;
}

void PlantInstance::getHurt(ZombieInstance *zombie, int aKind, int attack)
{
    if (aKind == 0)
        hp -= attack;
    if (hp < 1 || aKind != 0)
    {
        switch (qrand() % 5) {
            case 0: dieMusic->setMedia(QUrl("qrc:/audio/CaoFeng.mp3")); break;
            case 1: dieMusic->setMedia(QUrl("qrc:/audio/WaitForDeath.mp3")); break;
            case 2: dieMusic->setMedia(QUrl("qrc:/audio/BCML.mp3")); break;
            case 3: dieMusic->setMedia(QUrl("qrc:/audio/AttackFinish.mp3")); break;
            default: dieMusic->setMedia(QUrl("qrc:/audio/RuiZiEi.mp3")); break;
        }
        dieMusic->play();
        plantProtoType->scene->plantDie(this);
    }
}

Peashooter::Peashooter()
{
    eName = "oPeashooter";
    cName = tr("Peashooter");
    beAttackedPointR = 51;
    sunNum = 100;
    cardGif = "Card/Plants/Peashooter.png";
    staticGif = "Plants/Peashooter/0.gif";
    normalGif = "Plants/Peashooter/Peashooter.gif";
    toolTip = tr("Peashooters are your first line of defense. They shoot peas at attacking zombies.\n\nHow can a single plant grow and shoot so many peas so quickly? Peashooter says, \"Hard work, commitment, and a healthy, well-balanced breakfast of sunlight and high-fiber carbon dioxide make it all possible.\"");
    //?????????????????????????????????????????????????????????????????????????????????\n???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
}

Repeater::Repeater()
{
    eName = "oRepeater";
    cName = tr("Repeater");
    beAttackedPointR = 51;
    sunNum = 200;
    cardGif = "Card/Plants/Repeater.png";
    staticGif = "Plants/Repeater/0.gif";
    normalGif = "Plants/Repeater/Repeater.gif";
    toolTip = tr("Repeater fires two peas at a time.\nRepeater is fierce. He's from the streets. He doesn't take attitude from anybody, plant or zombie, and he shoots peas to keep people at a distance. Secretly, though, Repeater yearns for love.");
    //?????????????????????????????????????????????\n???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
}

GatlingPea::GatlingPea()//170
{
    eName = "oGatlingPea";
    cName = tr("GatlingPea");
    beAttackedPointR = 51;
    sunNum = 450;
    coolTime = 50;
    cardGif = "Card/Plants/GatlingPea.png";
    staticGif = "Plants/GatlingPea/0.gif";
    normalGif = "Plants/GatlingPea/GatlingPea.gif";
    toolTip = tr("Gatling Peas shoot four peas at a time.\nGatling Pea's parents were concerned when he announced his intention to join the military. \"But honey, it's so dangerous!\" they said in unison. Gatling Pea refused to budge. \"Life is dangerous,\" he replied, eyes glinting with steely conviction.");
    //???????????????????????????????????????\n????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
}

SnowPea::SnowPea()
{
    eName = "oSnowPea";
    cName = tr("Snow Pea");
    bKind = -1;
    beAttackedPointR = 51;
    sunNum = 175;
    cardGif = "Card/Plants/SnowPea.png";
    staticGif = "Plants/SnowPea/0.gif";
    normalGif = "Plants/SnowPea/SnowPea.gif";
    toolTip = tr("Snow Peas shoot frozen peas that damage and slow the enemy.\nFolks often tell Snow Pea how \"cool\" he is, or exhort him to \"chill out.\" They tell him to \"stay frosty.\" Snow Pea just rolls his eyes. He's heard 'em all.");
    //?????????????????????????????????????????????????????????????????????????????????????????????\n????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
}

SunFlower::SunFlower()
{
    eName = "oSunflower";
    cName = tr("Sunflower");
    beAttackedPointR = 53;
    sunNum = 50;
    cardGif = "Card/Plants/SunFlower.png";
    staticGif = "Plants/SunFlower/0.gif";
    normalGif = "Plants/SunFlower/SunFlower1.gif";
    toolTip = tr("Sunflowers are essential for you to produce extra sun. Try planting as many as you can!\nSunflower can't resist bouncing to the beat. Which beat is that? Why, the life-giving jazzy rhythm of the Earth itself, thumping at a frequency only Sunflower can hear.");
    // ???????????????????????????????????????????????????????????????????????????????????????\n??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
}

SunFlowerInstance::SunFlowerInstance(const Plant *plant)
        : PlantInstance(plant),
          lightedGif("Plants/SunFlower/SunFlower2.gif")
{

}

void SunFlowerInstance::initTrigger()
{
    (new Timer(picture, 5000, [this] {
        QSharedPointer<std::function<void(void)> > generateSun(new std::function<void(void)>);
        *generateSun = [this, generateSun] {
            picture->setMovieOnNewLoop(lightedGif, [this, generateSun] {
                (new Timer(picture, 1000, [this, generateSun] {
                    if(plantProtoType->scene->gameStatus)   return;
                    auto sunGifAndOnFinished = plantProtoType->scene->newSun(25);//????????????
                    MoviePixmapItem *sunGif = sunGifAndOnFinished.first;
                    std::function<void(bool)> onFinished = sunGifAndOnFinished.second;
                    Coordinate &coordinate = plantProtoType->scene->getCoordinate();
                    double fromX = coordinate.getX(col) - sunGif->boundingRect().width() / 2 + 15,
                            toX = coordinate.getX(col) - qrand() % 80,
                            toY = coordinate.getY(row) - sunGif->boundingRect().height();
                    sunGif->setScale(0.6);
                    sunGif->setPos(fromX, toY - 25);
                    sunGif->start();
                    Animate(sunGif).move(QPointF((fromX + toX) / 2, toY - 50)).scale(0.9).speed(0.2).shape(
                                    QTimeLine::EaseOutCurve).finish()
                            .move(QPointF(toX, toY)).scale(1.0).speed(0.2).shape(QTimeLine::EaseInCurve).finish(
                                    onFinished);
                    picture->setMovieOnNewLoop(plantProtoType->normalGif, [this, generateSun] {
                        (new Timer(picture, 24000, [this, generateSun] {
                            (*generateSun)();
                        }))->start();
                    });
                }))->start();
            });
        };
        (*generateSun)();
    }))->start();
}

WallNut::WallNut()
{
    eName = "oWallNut";
    cName = tr("Wall-nut");
    hp = 4000;
    beAttackedPointR = 45;
    sunNum = 50;
    coolTime = 30;
    cardGif = "Card/Plants/WallNut.png";
    staticGif = "Plants/WallNut/0.gif";
    normalGif = "Plants/WallNut/WallNut.gif";
    toolTip = tr("Wall-nuts have hard shells which you can use to protect your other plants.\n\"People wonder how I feel about getting constantly chewed on by zombies,\" says Wall-nut. \"What they don't realize is that with my limited senses all I can feel is a kind of tingling, like a relaxing back rub.\"");
    //???????????????????????????????????????????????????????????????????????????\n????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
}

TallNut::TallNut()
{
    eName = "oTallNut";
    cName = tr("Tall-nut");
    hp = 8000;
    beAttackedPointR = 45;
    sunNum = 125;
    coolTime = 30;
    cardGif = "Card/Plants/TallNut.png";
    staticGif = "Plants/TallNut/0.gif";
    normalGif = "Plants/TallNut/TallNut.gif";
    toolTip = tr("Tall-nuts are heavy-duty wall plants that can't be vaulted or jumped over. \nPeople wonder if there's a rivalry between Wall-nut and Tall-nut. Tall-nut laughs a rich baritone laugh. \"How could there be anything between us? We are brothers. If you knew what Wall-nut has done for me...\" Tall-nut's voice trails off and he smiles knowingly.");
    //????????????????????????????????????????????????????????????\n??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
}

Pumpkin::Pumpkin()
{
    pKind = 2;
    eName = "oPumpkin";
    cName = tr("Pumpkin");
    hp = 4000;
    beAttackedPointR = 80;
    sunNum = 125;
    coolTime = 30;
    cardGif = "Card/Plants/PumpkinHead.png";
    staticGif = "Plants/PumpkinHead/0.gif";
    normalGif = "Plants/PumpkinHead/PumpkinHead1.gif";
    toolTip = tr("Pumpkins protect plants that are within their shells.\nPumpkin hasn't heard from his cousin Renfield lately. Apparently Renfield's a big star, some kind of... what was it... sports hero? Peggle Master? Pumpkin doesn't really get it. He just does his job.");
    // ????????????????????????????????????\n???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
}

Torchwood::Torchwood()
{
    eName = "oTorchwood";
    cName = tr("Torchwood");
    beAttackedPointR = 45;
    sunNum = 175;
    cardGif = "Card/Plants/Torchwood.png";
    staticGif = "Plants/Torchwood/0.gif";
    normalGif = "Plants/Torchwood/Torchwood.gif";
    toolTip = tr("Torchwoods turn peas that pass through them into fireballs that deal twice as much damage.\nEverybody likes and respects Torchwood. They like him for his integrity, for his steadfast friendship, for his ability to greatly maximize pea damage. But Torchwood has a secret: He can't read.");
    // ?????????????????????????????????????????????????????????????????????\n ???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
}

PotatoMine::PotatoMine()
{
    eName = "oPotatoMine";
    cName = tr("Potato Mine");
    beAttackedPointR = 50;
    sunNum = 25;
    coolTime = 30;
    cardGif = "Card/Plants/PotatoMine.png";
    staticGif = "Plants/PotatoMine/0.gif";
    normalGif = "Plants/PotatoMine/PotatoMineNotReady.gif";
    toolTip = tr("Some folks say Potato Mine is lazy, that he leaves everything to the last minute. Potato Mine says nothing. He's too busy thinking about his investment strategy.");
    //???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????\n???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
}

Squash::Squash()
{
    eName = "oSquash";
    cName = tr("Squash");
    beAttackedPointR = 50;
    sunNum = 50;
    coolTime = 30;
    cardGif = "Card/Plants/Squash.png";
    staticGif = "Plants/Squash/0.gif";
    normalGif = "Plants/Squash/Squash.gif";
    toolTip = tr("Squashes will smash the first zombie that gets close to it.\n\"I\'m ready!\" yells Squash. \"Let\'s do it! Put me in! There\'s nobody better! I\'m your guy! C\'mon! Whaddya waiting for? I need this!\"");
    //????????????????????????????????????\n?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
}

CherryBomb::CherryBomb()
{
    eName = "oCherryBomb";
    cName = tr("Cherry Bomb");
    beAttackedPointR = 45;
    sunNum = 150;
    coolTime = 50;
    cardGif = "Card/Plants/CherryBomb.png";
    staticGif = "Plants/CherryBomb/0.gif";
    normalGif = "Plants/CherryBomb/CherryBomb.gif";
    toolTip = tr("Cherry Bombs can blow up all zombies in an area. They have a short fuse so plant them near zombies.\n\"I wanna explode,\" says Cherry #1. \"No, let's detonate instead!\" says his brother, Cherry #2. After intense consultation they agree to explodonate.");
    // ??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????\n????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
}

Jalapeno::Jalapeno()
{
    eName = "oJalapeno";
    cName = tr("Jalapeno");
    beAttackedPointR = 45;
    sunNum = 125;
    coolTime = 50;
    cardGif = "Card/Plants/Jalapeno.png";
    staticGif = "Plants/Jalapeno/0.gif";
    normalGif = "Plants/Jalapeno/Jalapeno.gif";
    toolTip = tr("Jalapenos destroy an entire row of zombies.\n\"NNNNNGGGGG!!!!!!!!\" Jalapeno says. He's not going to explode, not this time. But soon. Oh, so soon. It's close. He knows it, he can feel it, his whole life's been leading up to this moment.");
    // ????????????????????????????????????\n??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
}

LawnCleaner::LawnCleaner()
{
    eName = "oLawnCleaner";
    cName = tr("Lawn Cleaner");
    beAttackedPointL = 0;
    beAttackedPointR = 71;
    sunNum = 0;
    staticGif = normalGif = "interface/LawnCleaner.png";
    canEat = 0;
    stature = 1;
    toolTip = tr("Normal lawn cleaner");
}

PoolCleaner::PoolCleaner()
{
    eName = "oPoolCleaner";
    cName = tr("Pool Cleaner");
    beAttackedPointR = 47;
    staticGif = normalGif = "interface/PoolCleaner.png";
    toolTip = tr("Pool Cleaner");
    update();
}

bool WallNut::canGrow(int x, int y) const
{
   if (x < 1 || x > 9 || y < 1 || y > 5)
        return false;
    if (scene->isCrater(x, y) || scene->isTombstone(x, y))
        return false;
    int groundType = scene->getGameLevelData()->LF[y];
    QMap<int, PlantInstance *> plants = scene->getPlant(x, y);
    if (groundType == 1)
        return !plants.contains(1) || plants[1]->plantProtoType->eName == "oWallNut";
    return plants.contains(0) && (!plants.contains(1) || plants[1]->plantProtoType->eName == "oWallNut");

}

bool TallNut::canGrow(int x, int y) const
{
   if (x < 1 || x > 9 || y < 1 || y > 5)
        return false;
    if (scene->isCrater(x, y) || scene->isTombstone(x, y))
        return false;
    int groundType = scene->getGameLevelData()->LF[y];
    QMap<int, PlantInstance *> plants = scene->getPlant(x, y);
    if (groundType == 1)
        return !plants.contains(1) || plants[1]->plantProtoType->eName == "oTallNut";
    return plants.contains(0) && (!plants.contains(1) || plants[1]->plantProtoType->eName == "oTallNut");

}

bool Pumpkin::canGrow(int x, int y) const
{
   if (x < 1 || x > 9 || y < 1 || y > 5)
        return false;
    if (scene->isCrater(x, y) || scene->isTombstone(x, y))
        return false;
    int groundType = scene->getGameLevelData()->LF[y];
    QMap<int, PlantInstance *> plants = scene->getPlant(x, y);
    if (groundType == 1)
        return true;
    return plants.contains(0);

}

void WallNutInstance::initTrigger()
{}

WallNutInstance::WallNutInstance(const Plant *plant)
    : PlantInstance(plant),
      hurtMusic(new QMediaPlayer(picture))
{
    hurtStatus = 0;
    hurtPoint1 = 2667;
    hurtPoint2 = 1333;
    crackedGif1 = "Plants/WallNut/Wallnut_cracked1.gif";
    crackedGif2 = "Plants/WallNut/Wallnut_cracked2.gif";
}

void WallNutInstance::getHurt(ZombieInstance *zombie, int aKind, int attack)
{
    PlantInstance::getHurt(zombie, aKind, attack);
    if (hp > 0) {
        if (hp < hurtPoint2) {
            if (hurtStatus < 2) {
                hurtStatus = 2;
                picture->setMovie(crackedGif2);
                picture->start();
                hurtMusic->setMedia(QUrl("qrc:/audio/NoFear.mp3"));
                hurtMusic->play();
            }
        }
        else if (hp < hurtPoint1) {
            if (hurtStatus < 1) {
                hurtStatus = 1;
                picture->setMovie(crackedGif1);
                picture->start();
                hurtMusic->setMedia(QUrl("qrc:/audio/Tuo.mp3"));
                hurtMusic->play();
            }
        }
    }
}

TallNutInstance::TallNutInstance(const Plant *plant)
    : WallNutInstance(plant)
{
    canVaulted = false;
    hurtPoint1 = 5333;
    hurtPoint2 = 2667;
    crackedGif1 = "Plants/TallNut/TallnutCracked1.gif";
    crackedGif2 = "Plants/TallNut/TallnutCracked2.gif";
}

PumpkinInstance::PumpkinInstance(const Plant *plant)
    : WallNutInstance(plant),
      backPicture(new MoviePixmapItem)
{
    hurtPoint1 = 2667;
    hurtPoint2 = 1333;
    crackedGif1 = "Plants/PumpkinHead/pumpkin_damage1.gif";
    crackedGif2 = "Plants/PumpkinHead/pumpkin_damage2.gif";
}

PumpkinInstance::~PumpkinInstance(){
    backPicture->deleteLater();
}

void PumpkinInstance::birth(int c, int r)
{
    PlantInstance::birth(c,r);

    picture->setZValue(picture->zValue()+1);
    picture->setPos(picture->pos()+QPointF(0,10));

    backPicture->setMovie("Plants/PumpkinHead/PumpkinHead2.gif");
    backPicture->setPos(picture->pos()+QPointF(0,0));
    backPicture->setZValue(picture->zValue()-2);
    plantProtoType->scene->addToGame(backPicture);
}

void TorchwoodInstance::initTrigger()
{}

TorchwoodInstance::TorchwoodInstance(const Plant *plant)
    : PlantInstance(plant)
{
}

PotatoMineInstance::PotatoMineInstance(const Plant *plant)
    : PlantInstance(plant)
{
    explosionSpudowGif = "Plants/PotatoMine/ExplosionSpudow.gif";
    mashedGif = "Plants/PotatoMine/PotatoMine_mashed.gif";
}

void PotatoMineInstance::initTrigger(){
    (new Timer(picture, 15000, [this] {
        picture->setMovie("Plants/PotatoMine/PotatoMine.gif");
        Trigger *trigger = new Trigger(this, attackedLX, attackedRX+30, 0, 0);
        triggers.insert(row, QList<Trigger *>{ trigger } );
        plantProtoType->scene->addTrigger(row, trigger);//??????????????????????????????
    }))->start();
}

void PotatoMineInstance::triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger)//??????
{
    if (zombieInstance->altitude > 0) {
        normalAttack();
        getHurt(zombieInstance, 1, 300);
    }
}

void PotatoMineInstance::normalAttack(){
    MoviePixmapItem* explosionPixmap = new MoviePixmapItem;
    explosionPixmap->setPos(picture->pos());
    explosionPixmap->setZValue(picture->zValue()+1);
    explosionPixmap->setMovie(explosionSpudowGif);
    explosionPixmap->start();
    MoviePixmapItem* mashedPixmap = new MoviePixmapItem;
    mashedPixmap->setPos(picture->pos());
    mashedPixmap->setZValue(picture->zValue());
    mashedPixmap->setMovie(mashedGif);
    mashedPixmap->start();
    plantProtoType->scene->addToGame(explosionPixmap);
    plantProtoType->scene->addToGame(mashedPixmap);
    QMediaPlayer *boomMusic = new QMediaPlayer(plantProtoType->scene);
    boomMusic->setMedia(QUrl("qrc:/audio/potato_mine.mp3"));
    boomMusic->play();
    (new Timer(plantProtoType->scene, 2500, [explosionPixmap, mashedPixmap, boomMusic] {
        explosionPixmap->deleteLater();
        mashedPixmap->deleteLater();
        boomMusic->deleteLater();
    }))->start();
    Coordinate &coordinate = plantProtoType->scene->getCoordinate();
    double x = coordinate.getX(col);
    QList<ZombieInstance *> zombies = plantProtoType->scene->getZombieOnRow(row);
    for (auto iter = zombies.begin(); iter != zombies.end();iter++) {
        if ((*iter)->hp > 0 && (*iter)->attackedRX >= x-80 && (*iter)->attackedLX <= x+80) {
            (*iter)->getBoom(1800);
        }
    }
}

SquashInstance::SquashInstance(const Plant *plant)
    : PlantInstance(plant)
{
    attackGif = "Plants/Squash/SquashAttack.gif";
}

void SquashInstance::initTrigger(){
     Trigger *trigger = new Trigger(this, attackedLX-50, attackedRX+100, 0, 0);
     triggers.insert(row, QList<Trigger *>{ trigger } );
     plantProtoType->scene->addTrigger(row, trigger);
}

void SquashInstance::triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger)//??????
{
    if (zombieInstance->altitude > 0) {
        target = zombieInstance->X;
        canTrigger = false;
        normalAttack();
    }
}

void SquashInstance::normalAttack(){
    QMediaPlayer *hmmMusic = new QMediaPlayer(plantProtoType->scene);
    hmmMusic->setMedia(QUrl("qrc:/audio/ZheBo.mp3"));
    hmmMusic->play();
    picture->setMovie("Plants/Squash/SquashR.png");
    picture->start();
    (new Timer(picture, 1100, [this, hmmMusic]{
        hmmMusic->setMedia(QUrl("qrc:/audio/MeatEggShort.mp3"));
        hmmMusic->play();
    }))->start();
    (new Timer(picture, 1500, [this, hmmMusic]{
        picture->hide();
        attackedLX=-30;
        attackedRX=-30;
        MoviePixmapItem* mashedPixmap = new MoviePixmapItem;
        mashedPixmap->setPos(target,picture->y());
        mashedPixmap->setZValue(picture->zValue());
        mashedPixmap->setMovie(attackGif);
        mashedPixmap->start();
        plantProtoType->scene->addToGame(mashedPixmap);
        QMediaPlayer *crashMusic = new QMediaPlayer(plantProtoType->scene);
        crashMusic->setMedia(QUrl("qrc:/audio/gargantuar_thump.mp3"));
        crashMusic->play();
        (new Timer(plantProtoType->scene, 2300, [mashedPixmap, hmmMusic, crashMusic] {
            mashedPixmap->deleteLater();
            hmmMusic->deleteLater();
            crashMusic->deleteLater();
        }))->start();
        qDebug()<<"crash 1";
        (new Timer(picture, 120, [this]{
            QList<ZombieInstance *> zombies = plantProtoType->scene->getZombieOnRow(row);
            for (auto iter = zombies.begin(); iter != zombies.end();iter++) {
                if ((*iter)->hp > 0 && (*iter)->attackedRX >= target+10 && (*iter)->attackedLX <= target+105) {
                    //(*iter)->getHit(1800);
                    plantProtoType->scene->zombieDie(*iter);
                }
            }
            getHurt(nullptr, 1, 300);
        }))->start();
    }))->start();
}

CherryBombInstance::CherryBombInstance(const Plant *plant)
    : PlantInstance(plant)
{
    BoomGif = "Plants/CherryBomb/Boom.gif";
}

void CherryBombInstance::initTrigger(){
    (new Timer(picture, 620, [this] {
        normalAttack();
        getHurt(nullptr, 1, 300);
    }))->start();
}

void CherryBombInstance::triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger)//??????
{
}

void CherryBombInstance::normalAttack(){
    MoviePixmapItem* boomPixmap = new MoviePixmapItem;
    boomPixmap->setPos(picture->pos()+QPointF(-10,-25));
    boomPixmap->setZValue(picture->zValue());
    boomPixmap->setMovie(BoomGif);
    boomPixmap->start();
    plantProtoType->scene->addToGame(boomPixmap);
    QMediaPlayer *boomMusic = new QMediaPlayer(plantProtoType->scene);
    boomMusic->setMedia(QUrl("qrc:/audio/explosion.mp3"));
    boomMusic->play();
    (new Timer(plantProtoType->scene, 2500, [boomPixmap, boomMusic] {
        boomPixmap->deleteLater();
        boomMusic->deleteLater();
    }))->start();
    Coordinate &coordinate = plantProtoType->scene->getCoordinate();
    double x = coordinate.getX(col);
    for (int r = row>1?row-1:1; r <= coordinate.rowCount() && r <= row + 1; ++r) {
        QList<ZombieInstance *> zombies = plantProtoType->scene->getZombieOnRow(r);
        for (auto iter = zombies.begin(); iter != zombies.end();iter++) {
            if ((*iter)->hp > 0 && (*iter)->attackedRX >= x-120 && (*iter)->attackedLX <= x+120) {
                (*iter)->getBoom(1800);
            }
        }
    }
}

JalapenoInstance::JalapenoInstance(const Plant *plant)
    : PlantInstance(plant)
{
    BoomGif = "Plants/Jalapeno/JalapenoAttack.gif";
}

void JalapenoInstance::initTrigger(){
    (new Timer(picture, 620, [this] {
        normalAttack();
        getHurt(nullptr, 1, 300);
    }))->start();
}

void JalapenoInstance::triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger)//??????
{
}

void JalapenoInstance::normalAttack(){
    MoviePixmapItem* boomPixmap = new MoviePixmapItem;
    boomPixmap->setX(plantProtoType->scene->getCoordinate().getX(0));
    boomPixmap->setY(plantProtoType->scene->getCoordinate().getY(row)-165);
    boomPixmap->setZValue(picture->zValue());
    boomPixmap->setMovie(BoomGif);
    boomPixmap->start();
    plantProtoType->scene->addToGame(boomPixmap);
    QMediaPlayer *boomMusic = new QMediaPlayer(plantProtoType->scene);
    boomMusic->setMedia(QUrl("qrc:/audio/explosion.mp3"));
    boomMusic->play();
    (new Timer(plantProtoType->scene, 2500, [boomPixmap, boomMusic] {
        boomPixmap->deleteLater();
        boomMusic->deleteLater();
    }))->start();
    Coordinate &coordinate = plantProtoType->scene->getCoordinate();
    double x = coordinate.getX(col);
    QList<ZombieInstance *> zombies = plantProtoType->scene->getZombieOnRow(row);
    for (auto iter = zombies.begin(); iter != zombies.end();iter++) {
        if ((*iter)->hp > 0) {
            (*iter)->getBoom(1800);
        }
    }
}

LawnCleanerInstance::LawnCleanerInstance(const Plant *plant)
    : PlantInstance(plant)
{}

void LawnCleanerInstance::initTrigger()
{
    Trigger *trigger = new Trigger(this, attackedLX, attackedRX, 0, 0);
    triggers.insert(row, QList<Trigger *>{ trigger } );
    plantProtoType->scene->addTrigger(row, trigger);
}

void LawnCleanerInstance::triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger)
{
    if (zombieInstance->beAttacked && zombieInstance->altitude > 0) {
        canTrigger = 0;
        normalAttack(nullptr);
    }
}

void LawnCleanerInstance::normalAttack(ZombieInstance *zombieInstance)
{
    QMediaPlayer *player = new QMediaPlayer(plantProtoType->scene);
    player->setMedia(QUrl("qrc:/audio/lawnmower.mp3"));
    player->play();
    QSharedPointer<std::function<void(void)> > crush(new std::function<void(void)>);
    *crush = [this, crush] {
        for (auto zombie: plantProtoType->scene->getZombieOnRowRange(row, attackedLX, attackedRX)) {
            zombie->crushDie();
        }
        if (attackedLX > 900)
            plantProtoType->scene->plantDie(this);
        else {
            attackedLX += 10;
            attackedRX += 10;
            picture->setPos(picture->pos() + QPointF(10, 0));
            (new Timer(picture, 10, *crush))->start();
        }
    };
    (*crush)();
}

PeashooterInstance::PeashooterInstance(const Plant *plant)
    : PlantInstance(plant)
{
}

void PeashooterInstance::normalAttack(ZombieInstance *zombieInstance)
{
    attackMusic->setMedia(QUrl("qrc:/audio/dian.mp3"));
    attackMusic->play();
    (new Bullet(plantProtoType->scene, 0, row, attackedLX, attackedLX - 40, picture->y() + 3, picture->zValue() + 2, 0))->start();
}

RepeaterInstance::RepeaterInstance(const Plant *plant)
    : PeashooterInstance(plant)
{
}

void RepeaterInstance::normalAttack(ZombieInstance *zombieInstance)
{
    attackMusic->setMedia(QUrl("qrc:/audio/diandian.mp3"));
    attackMusic->play();
    (new Timer(picture, 300, [this] {
        (new Bullet(plantProtoType->scene, 0, row, attackedLX, attackedLX - 40, picture->y() + 3, picture->zValue() + 2, 0))->start();
    }))->start();
    (new Bullet(plantProtoType->scene, 0, row, attackedLX, attackedLX - 40, picture->y() + 3, picture->zValue() + 2, 0))->start();
}

GatlingPeaInstance::GatlingPeaInstance(const Plant *plant)
    : PeashooterInstance(plant)
{
}

void GatlingPeaInstance::normalAttack(ZombieInstance *zombieInstance)
{
    attackMusic->setMedia(QUrl("qrc:/audio/Gatling.mp3"));
    attackMusic->play();
    (new Timer(picture, 510, [this] {
        (new Bullet(plantProtoType->scene, 0, row, attackedLX, attackedLX - 40, picture->y() + 3, picture->zValue() + 2, 0))->start();
    }))->start();
    (new Timer(picture, 340, [this] {
        (new Bullet(plantProtoType->scene, 0, row, attackedLX, attackedLX - 40, picture->y() + 3, picture->zValue() + 2, 0))->start();
    }))->start();
    (new Timer(picture, 170, [this] {
        (new Bullet(plantProtoType->scene, 0, row, attackedLX, attackedLX - 40, picture->y() + 3, picture->zValue() + 2, 0))->start();
    }))->start();
    (new Bullet(plantProtoType->scene, 0, row, attackedLX, attackedLX - 40, picture->y() + 3, picture->zValue() + 2, 0))->start();
}

SnowPeaInstance::SnowPeaInstance(const Plant *plant)
    : PeashooterInstance(plant)
{
}

void SnowPeaInstance::normalAttack(ZombieInstance *zombieInstance)
{
    attackMusic->setMedia(QUrl("qrc:/audio/huanshu.mp3"));
    attackMusic->play();
    (new Bullet(plantProtoType->scene, -1, row, attackedLX, attackedLX - 40, picture->y() + 3, picture->zValue() + 2, 0))->start();
}


Bullet::Bullet(GameScene *scene, int type, int row, qreal from, qreal x, qreal y, qreal zvalue, int direction)
        : scene(scene), type(type), row(row), direction(direction), from(from), beFired(false)
{
    count = 0;
    picture = new QGraphicsPixmapItem(gImageCache->load(QString("Plants/PB%1%2.gif").arg(type).arg(direction)));
    picture->setPos(x, y);
    picture->setZValue(zvalue);
}

Bullet::~Bullet()
{
    delete picture;
}

void Bullet::start()
{
    (new Timer(scene, 10, [this] {
        move();
    }))->start();
}

void Bullet::move()
{
    if (count++ == 10)
        scene->addItem(picture);
    int col = scene->getCoordinate().getCol(from);
    QMap<int, PlantInstance *> plants = scene->getPlant(col, row);
    if (!beFired && type < 1 && plants.contains(1) && plants[1]->plantProtoType->eName == "oTorchwood") {
        ++type;
        beFired = true;
        (new Timer(scene, 210, [this] {
            beFired = false;
        }))->start();
        picture->setPixmap(gImageCache->load(QString("Plants/PB%1%2.gif").arg(type).arg(direction)));
    }
    ZombieInstance *zombie = nullptr;
    if (direction == 0) {
        QList<ZombieInstance *> zombies = scene->getZombieOnRow(row);
        for (auto iter = zombies.end(); iter-- != zombies.begin() && (*iter)->attackedLX <= from;) {
            if ((*iter)->hp > 0 && (*iter)->attackedRX >= from) {
                zombie = *iter;
                break;
            }
        }
    }
    // TODO: another direction
    if (zombie && zombie->altitude == 1) {
        if(type==1)     zombie->getPea(40, direction, 1);
        else            zombie->getPea(20, direction, type);
        picture->setPos(picture->pos() + QPointF(28, 0));
        picture->setPixmap(gImageCache->load("Plants/PeaBulletHit.gif"));
        (new Timer(scene, 100, [this] {
            delete this;
        }))->start();
    }
    else {
        from += direction ? -5 : 5;
        if (from < 900 && from > 100) {
            picture->setPos(picture->pos() + QPointF(direction ? -5 : 5, 0));
            (new Timer(scene, 10, [this] {
                move();
            }))->start();
        }
        else
            delete this;
    }
}

Plant *PlantFactory(GameScene *scene, const QString &eName)
{
    Plant *plant = nullptr;
    if (eName == "oPeashooter")
        plant = new Peashooter;
    else if (eName == "oRepeater")
        plant = new Repeater;
    else if (eName == "oGatlingPea")
        plant = new GatlingPea;
    else if (eName == "oSnowPea")
        plant = new SnowPea;
    else if (eName == "oSunflower")
        plant = new SunFlower;
    else if (eName == "oWallNut")
        plant = new WallNut;
    else if (eName == "oTallNut")
        plant = new TallNut;
    else if (eName == "oPumpkin")
        plant = new Pumpkin;
    else if (eName == "oPotatoMine")
        plant = new PotatoMine;
    else if (eName == "oSquash")
        plant = new Squash;
    else if (eName == "oCherryBomb")
        plant = new CherryBomb;
    else if (eName == "oJalapeno")
        plant = new Jalapeno;
    else if (eName == "oTorchwood")
        plant = new Torchwood;
    else if (eName == "oLawnCleaner")
        plant = new LawnCleaner;
    else if (eName == "oPoolCleaner")
        plant = new PoolCleaner;
    if (plant) {
        plant->scene = scene;
        plant->update();
    }
    return plant;
}

PlantInstance *PlantInstanceFactory(const Plant *plant)
{
    if (plant->eName == "oPeashooter")
        return new PeashooterInstance(plant);
    else if (plant->eName == "oRepeater")
        return new RepeaterInstance(plant);
    else if (plant->eName == "oGatlingPea")
        return new GatlingPeaInstance(plant);
    else if (plant->eName == "oSnowPea")
        return new SnowPeaInstance(plant);
    else if (plant->eName == "oSunflower")
        return new SunFlowerInstance(plant);
    else if (plant->eName == "oWallNut")
        return new WallNutInstance(plant);
    else if (plant->eName == "oTallNut")
        return new TallNutInstance(plant);
    else if (plant->eName == "oPumpkin")
        return new PumpkinInstance(plant);
    else if (plant->eName == "oPotatoMine")
        return new PotatoMineInstance(plant);
    else if (plant->eName == "oSquash")
        return new SquashInstance(plant);
    else if (plant->eName == "oCherryBomb")
        return new CherryBombInstance(plant);
    else if (plant->eName == "oJalapeno")
        return new JalapenoInstance(plant);
    else if (plant->eName == "oTorchwood")
        return new TorchwoodInstance(plant);
    else if (plant->eName == "oLawnCleaner")
        return new LawnCleanerInstance(plant);
    return new PlantInstance(plant);
}



