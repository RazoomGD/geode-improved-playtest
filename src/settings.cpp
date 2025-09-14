#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/EditorPauseLayer.hpp>


class PlaytestCameraSettingsPopup : public Popup<int> {
public:
    const float m_width = 200.f;
    const float m_height = 190.f;
    float m_currPos = -30.f;
    Ref<CCMenuItemToggler> m_pauseMenuToggler{};

    CCMenuItemToggler* m_enabled;
    CCMenuItemToggler* m_staticPos;
    CCMenuItemToggler* m_staticZoom;
    CCMenuItemToggler* m_staticRot;
    CCMenuItemToggler* m_windowRect;

    static PlaytestCameraSettingsPopup* create() {
        auto ret = new PlaytestCameraSettingsPopup();
        if (ret && ret->initAnchored(ret->m_width, ret->m_height, 0)) {
            ret->autorelease();
            return ret; 
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    CCMenuItemToggler* createToggler(const char *lab, int tag) {
        auto ret = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(PlaytestCameraSettingsPopup::onToggler), 0.6);
        m_buttonMenu->addChildAtPosition(ret, Anchor::TopLeft, ccp(30, m_currPos));
        ret->setTag(tag);
        ret->setCascadeOpacityEnabled(true);
        ret->setCascadeColorEnabled(true);
        ret->updateSprite();
        auto label = CCLabelBMFont::create(lab, "bigFont.fnt");
        label->limitLabelWidth(m_width - 70, 0.55, 0);
        label->setAnchorPoint({0,0.5});
        m_buttonMenu->addChildAtPosition(label, Anchor::TopLeft, ccp(50, m_currPos));
        m_currPos -= 25;
        return ret;
    }

    void createSeparator() {
        auto vline = CCSprite::createWithSpriteFrameName("edit_vLine_001.png");
        vline->setScaleY(2);
        vline->setColor(ccc3(100, 50, 0));
        vline->setOpacity(100);
        vline->setRotation(90);
        m_buttonMenu->addChildAtPosition(vline, Anchor::Top, ccp(0, m_currPos + 7.5));
        m_currPos -= 10;
    }

    bool setup(int) {
        m_closeBtn->setVisible(false);
        // setTitle("Playtest Camera Settings");

        m_buttonMenu->addChildAtPosition(
            InfoAlertButton::create(
                "Playtest Camera Settings", 
                "<cy>Change the behavior of the camera during playtest:</c>\n"
                "- <cj>Enable</c>: enable/disable the mod.\n"
                "- <cp>Static position</c>: ignore any camera movement.\n"
                "- <cp>Static zoom</c>: ignore any camera zooming.\n"
                "- <cp>Static rotation</c>: ignore any camera rotation.\n"
                "- <co>Show edges</c>: show original camera bounds.\n", 
                0.75
            ), 
            Anchor::TopRight, ccp(-18, -18)
        );

        m_buttonMenu->addChildAtPosition(
            CCMenuItemSpriteExtra::create(
                ButtonSprite::create("ok", "goldFont.fnt", "GJ_button_01.png", 1),
                this, menu_selector(PlaytestCameraSettingsPopup::onClose)
            ),
            Anchor::Bottom
        );
        
        m_enabled = createToggler("Enable", 1);
        createSeparator();
        m_staticPos = createToggler("Static position", 2);
        m_staticZoom = createToggler("Static zoom", 3);
        m_staticRot = createToggler("Static rotation", 4);
        createSeparator();
        m_windowRect = createToggler("Show edges", 5);

        m_enabled->toggle(Mod::get()->getSavedValue<bool>("enabled", true));
        m_staticPos->toggle(Mod::get()->getSavedValue<bool>("static_pos", true));
        m_staticZoom->toggle(Mod::get()->getSavedValue<bool>("static_zoom", true));
        m_staticRot->toggle(Mod::get()->getSavedValue<bool>("static_rot", true));
        m_windowRect->toggle(Mod::get()->getSavedValue<bool>("window_rect", true));

        updateButtonVisibility(m_enabled->isOn());

        setID("playtest-camera-settings-popup"_spr);
        return true;
    }

    void updateButtonVisibility(bool isOn) {
        auto op = isOn ? 255 : 150;
        auto col = isOn ? ccc3(255,255,255) : ccc3(150,150,150);
        CCMenuItemToggler* arr[] = {m_staticPos, m_staticZoom, m_staticRot, m_windowRect};
        for (int i = 0; i < 4; i++) {
            arr[i]->setOpacity(op);
            arr[i]->setColor(col);
            arr[i]->setEnabled(isOn);
        }
    }

    void onToggler(CCObject* sender) {
        // sender->isOn() - is the PREVIOUS state of the toggler!
        auto btn = static_cast<CCMenuItemToggler*>(sender);
        bool on = !btn->isOn();
        if (btn->getTag() == 1) {
            if (m_pauseMenuToggler) m_pauseMenuToggler->toggle(on);
            updateButtonVisibility(on);
        }
    }

    void onClose(CCObject* obj) override {
        Mod::get()->setSavedValue<bool>("enabled", m_enabled->isOn());
        Mod::get()->setSavedValue<bool>("static_pos", m_staticPos->isOn());
        Mod::get()->setSavedValue<bool>("static_zoom", m_staticZoom->isOn());
        Mod::get()->setSavedValue<bool>("static_rot", m_staticRot->isOn());
        Mod::get()->setSavedValue<bool>("window_rect", m_windowRect->isOn());
        Popup::onClose(obj);
    }
};



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

        auto lab = CCLabelBMFont::create("Static Camera", "bigFont.fnt");
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

