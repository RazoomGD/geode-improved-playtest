#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>


class $modify(MyGJBaseGameLayer, GJBaseGameLayer) {

	struct Fields {
		bool m_enabled{};
		bool m_staticCameraEnabled{};
		bool m_staticZoomEnabled{};
		bool m_staticRotEnabled{};
		bool m_debugDrawEnabled{};

		CCPoint m_staticCenterPos{}; // in Editor coords
		float m_staticZoom{};

		CCDrawNode* m_drawWinRectNode;
	};


	void setupModDebugDrawNode() {
		m_fields->m_drawWinRectNode = CCDrawNode::create();
		m_fields->m_drawWinRectNode->setID("debug-draw-node"_spr);
		m_debugDrawNode->getParent()->addChild(m_fields->m_drawWinRectNode, 3000);
	}


	void visit() {
		auto f = m_fields.self();
		if (f->m_enabled) {
			bool ground = m_groundLayer->isVisible();
			bool ground2 = m_groundLayer2->isVisible();
			m_groundLayer->setVisible(false);
			m_groundLayer2->setVisible(false);
			// I have NO clue why, but RobTop handles Camera rotation in visit()
			if (f->m_staticRotEnabled) {
				float angle = m_gameState.m_cameraAngle;
				m_gameState.m_cameraAngle = 0;
				GJBaseGameLayer::visit();
				m_gameState.m_cameraAngle = angle;
			} else {
				GJBaseGameLayer::visit();
			}
			m_groundLayer->setVisible(ground);
			m_groundLayer2->setVisible(ground2);
		} else {
			GJBaseGameLayer::visit();
		}
	}


	inline void setScalePos(CCPoint pos, float scale) {
		auto debugDrawLayer = m_debugDrawNode->getParent();
		debugDrawLayer->setScale(scale);
		debugDrawLayer->setPosition(pos);
		m_objectLayer->setScale(scale);
		m_objectLayer->setPosition(pos);
	}


	void playtestCameraUpdate() {
		auto f = m_fields.self();
		if (!f->m_enabled) return;
		
		auto winSz = CCDirector::get()->getWinSize();
		auto scaledSz = winSz / m_gameState.m_cameraZoom;
		auto camCenter = m_gameState.m_cameraPosition + scaledSz / 2;
		
		if (f->m_debugDrawEnabled) {
			float angleRad = m_gameState.m_cameraAngle / 180.f * M_PI;
			float cosA = std::cos(angleRad);
			float sinA = std::sin(angleRad);
		
			float hw = scaledSz.width / 2.f;
			float hh = scaledSz.height / 2.f;
			CCPoint points[] = {{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}};
		
			for (int i = 0; i < 4; ++i) {
				float xr = points[i].x * cosA - points[i].y * sinA;
				float yr = points[i].x * sinA + points[i].y * cosA;
				points[i] = camCenter + ccp(xr, yr);
			}
		
			f->m_drawWinRectNode->clear();
			f->m_drawWinRectNode->drawPolygon(points, 4, ccc4f(0,0,0,0), 1, ccc4f(1,1,0,1));
		}

		auto zoom = f->m_staticZoomEnabled ? f->m_staticZoom : m_gameState.m_cameraZoom;
		auto center = f->m_staticCameraEnabled ? f->m_staticCenterPos : camCenter;
		auto newCamPos = center * zoom - winSz / 2;
		setScalePos(-newCamPos, zoom);
	}


	void update(float p0) {
		GJBaseGameLayer::update(p0);
		playtestCameraUpdate();
	}


	void restoreStaticCameraPos() {
		auto f = m_fields.self();
		if (!f->m_enabled || !f->m_staticCameraEnabled) return;

		auto zoom = f->m_staticZoomEnabled ? f->m_staticZoom : m_objectLayer->getScale();
		auto newCamPos = f->m_staticCenterPos * zoom - CCDirector::get()->getWinSize() / 2;
		setScalePos(-newCamPos, zoom);
	}


	void updateCameraBGArt(CCPoint p0, float p1) {
		auto f = m_fields.self();
		if (f->m_enabled) {
			CCPoint oldPos = m_background->getPosition();
			float oldZoom = m_background->getScale();
			GJBaseGameLayer::updateCameraBGArt(p0, p1);
			if (f->m_staticCameraEnabled) {
				m_background->setPosition(oldPos);
			}
			if (f->m_staticZoomEnabled) {
				m_background->setScale(oldZoom);
			}
		} else {
			GJBaseGameLayer::updateCameraBGArt(p0, p1);
		}
	}
};


class $modify(LevelEditorLayer) {
	void updateVisibility(float p0) {
		auto f = reinterpret_cast<MyGJBaseGameLayer*>(this)->m_fields.self();
		if (f->m_enabled) {
			// m_unkPoint33 - bottom left camera corner in editor coords
			auto oldZoom = m_gameState.m_cameraZoom;
			auto oldAngle = m_gameState.m_cameraAngle;
			auto oldUnk33 = m_gameState.m_unkPoint33;

			if (f->m_staticZoomEnabled) m_gameState.m_cameraZoom = f->m_staticZoom;
			if (f->m_staticRotEnabled) m_gameState.m_cameraAngle = 0;
			if (f->m_staticCameraEnabled) {
				auto winCenterInEditorScale = CCDirector::get()->getWinSize() / 2 / m_gameState.m_cameraZoom;
				m_gameState.m_unkPoint33 = f->m_staticCenterPos - winCenterInEditorScale;
			}

			LevelEditorLayer::updateVisibility(p0);

			m_gameState.m_cameraZoom = oldZoom;
			m_gameState.m_cameraAngle = oldAngle;
			m_gameState.m_unkPoint33 = oldUnk33;

		} else {
			LevelEditorLayer::updateVisibility(p0);
		}
	}
};


class $modify(EditorUI) {

	bool init(LevelEditorLayer* editorLayer) {
		if (!EditorUI::init(editorLayer)) return false;
		reinterpret_cast<MyGJBaseGameLayer*>(m_editorLayer)->setupModDebugDrawNode();
		return true;
	}

	void updatePlaytestValues() {
		auto zoom = m_editorLayer->m_objectLayer->getScale();
		auto camCenter = (CCDirector::get()->getWinSize() / 2 - m_editorLayer->m_objectLayer->getPosition()) / zoom;
		
		auto f = reinterpret_cast<MyGJBaseGameLayer*>(m_editorLayer)->m_fields.self();
		
		f->m_enabled = Mod::get()->getSavedValue<bool>("enabled", true);
		f->m_staticCameraEnabled = Mod::get()->getSavedValue<bool>("static_pos", true);
		f->m_staticZoomEnabled = Mod::get()->getSavedValue<bool>("static_zoom", true);
		f->m_staticRotEnabled = Mod::get()->getSavedValue<bool>("static_rot", true);
		f->m_debugDrawEnabled = Mod::get()->getSavedValue<bool>("window_rect", true);
		
		f->m_staticCenterPos = camCenter;
		f->m_staticZoom = zoom;
	}
	
	void destroyPlaytestValues() {
		auto f = reinterpret_cast<MyGJBaseGameLayer*>(m_editorLayer)->m_fields.self();
		f->m_enabled = false;
		if (f->m_drawWinRectNode) f->m_drawWinRectNode->clear();
	}

	void onPlaytest(CCObject* sender) {
		updatePlaytestValues();
		EditorUI::onPlaytest(sender);
		if (m_editorLayer->m_playbackMode != PlaybackMode::Playing) {
			destroyPlaytestValues();
		}
	}

	void playtestStopped() {
		EditorUI::playtestStopped();
		reinterpret_cast<MyGJBaseGameLayer*>(m_editorLayer)->restoreStaticCameraPos();
		destroyPlaytestValues();
	}
};

// todo: 
// 1. fix bg jump
// 2. fix mg pos
