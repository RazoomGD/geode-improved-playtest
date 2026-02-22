#include "settings.hpp"

class $modify(MyPauseLayer, EditorPauseLayer) {

    struct Fields {
        CCMenuItemToggler* m_toggler;
    };

    static void onModify(auto& self) {
        if (!self.setHookPriority("EditorPauseLayer::init", Priority::Late)) {
            log::warn("Failed to set hook priority");
        }
    }
    
    bool init(LevelEditorLayer* p0) {
        if (!EditorPauseLayer::init(p0)) return false;
        auto menu = getChildByID("options-menu");
        auto myMenu = CCMenu::create();
        myMenu->setID("togglers"_spr);

        auto toggler = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(MyPauseLayer::onModToggler), 0.55);
        myMenu->addChild(toggler);
        toggler->setPosition({9.625, 10});
        toggler->updateSprite();
        toggler->toggle(Mod::get()->getSavedValue<bool>("enabled", true));
        m_fields->m_toggler = toggler;        

        auto lab = CCLabelBMFont::create("Improved Playtest", "bigFont.fnt");
        lab->setAnchorPoint({0,0.5});
        lab->limitLabelWidth(90, 1, 0);
        myMenu->addChild(lab);
        lab->setPosition({24.25, 10});

        auto btn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_plus3Btn_001.png"),
            this, menu_selector(MyPauseLayer::onModOptions)
        );
        myMenu->addChild(btn);
        btn->setPosition({124, 10});
        btn->setScale(0.8);
        btn->m_baseScale = 0.8;
        
        myMenu->setContentSize({120, 20});
        menu->addChild(myMenu, -1);
        auto origBb = menu->boundingBox();
        menu->setContentHeight(menu->getContentHeight() + 30);
        if (menu->getScaledContentHeight() > 265) {
            menu->setScale(menu->getScale() / menu->getScaledContentHeight() * 265);
        }
        auto newBb = menu->boundingBox();
        menu->setPositionY(menu->getPositionY() + origBb.getMinY() - newBb.getMinY());
        menu->setPositionX(menu->getPositionX() + origBb.getMinX() - newBb.getMinX());
        menu->updateLayout();
        return true;
    }

    void onModToggler(CCObject* btn) {
        // sender->isOn() - is the PREVIOUS state of the toggler!
        bool on = !static_cast<CCMenuItemToggler*>(btn)->isOn();
        Mod::get()->setSavedValue("enabled", on);
    }

    void onModOptions(CCObject* btn) {
        auto popup = PlaytestCameraSettingsPopup::create();
        popup->m_pauseMenuToggler = m_fields->m_toggler;
        popup->show();
    }
};