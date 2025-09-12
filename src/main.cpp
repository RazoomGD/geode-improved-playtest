#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>




class $modify(MyGJBaseGameLayer, GJBaseGameLayer) {

	struct Fields {
		bool m_enabled{};
		bool m_staticCameraEnabled{};
		bool m_staticZoomEnabled{};
		bool m_staticRotEnabled{};
		bool m_debugDrawEnabled{};

		CCPoint m_staticCenterPos{};
		float m_staticZoom{};

		CCDrawNode* m_drawWinRectNode;
	};

	void setupLayers() {
		GJBaseGameLayer::setupLayers();
		auto dn = CCDrawNode::create();
		dn->setID("draw-win-rect-node"_spr);
		m_objectLayer->addChild(dn, 3000);
		m_fields->m_drawWinRectNode = dn;
	}

	void visit() {
		// I have NO clue why, but RobTop handles Camera rotation in visit()
		auto f = m_fields.self();
		if (f->m_enabled && f->m_staticRotEnabled) {
			float tmp = m_gameState.m_cameraAngle;
			m_gameState.m_cameraAngle = 0;
			GJBaseGameLayer::visit();
			m_gameState.m_cameraAngle = tmp;
		} else {
			GJBaseGameLayer::visit();
		}
	}

	void update(float p0) {
		GJBaseGameLayer::update(p0);
		
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
				double xr = points[i].x * cosA - points[i].y * sinA;
				double yr = points[i].x * sinA + points[i].y * cosA;
				points[i] = camCenter + ccp(xr, yr);
			}
		
			f->m_drawWinRectNode->clear();
			f->m_drawWinRectNode->drawPolygon(points, 4, ccc4f(0,0,0,0), 1, ccc4f(1,0,1,1));
		}
		
		if (f->m_staticZoomEnabled && f->m_staticCameraEnabled) {
			m_objectLayer->setScale(f->m_staticZoom);
			auto newCamPos = f->m_staticCenterPos * f->m_staticZoom - winSz / 2;
			m_objectLayer->setPosition(-newCamPos);

		} else if (f->m_staticZoomEnabled) {
			m_objectLayer->setScale(f->m_staticZoom);
			auto newCamPos = camCenter * f->m_staticZoom - winSz / 2;
			m_objectLayer->setPosition(-newCamPos);

		} else if (f->m_staticCameraEnabled) {
			auto newCamPos = f->m_staticCenterPos * m_gameState.m_cameraZoom - winSz / 2;
			m_objectLayer->setPosition(-newCamPos);
		}

		
	}



	
	
	void updateDebugDraw() {
		
		GJBaseGameLayer::updateDebugDraw();
	}

};


class $modify(LevelEditorLayer) {
	void onPlaytest() {
		auto zoom = m_objectLayer->getScale();
		auto camCenter = (CCDirector::get()->getWinSize() / 2 - m_objectLayer->getPosition()) / zoom;

		auto f = reinterpret_cast<MyGJBaseGameLayer*>(this)->m_fields.self();

		f->m_enabled = Mod::get()->getSavedValue<bool>("enabled", true);
		f->m_staticCameraEnabled = Mod::get()->getSavedValue<bool>("static_pos", true);
		f->m_staticZoomEnabled = Mod::get()->getSavedValue<bool>("static_zoom", true);
		f->m_staticRotEnabled = Mod::get()->getSavedValue<bool>("static_rot", true);
		f->m_debugDrawEnabled = Mod::get()->getSavedValue<bool>("window_rect", true);

		f->m_staticCenterPos = camCenter;
		f->m_staticZoom = zoom;

		log::info("Cam cent: {}", camCenter);

		LevelEditorLayer::onPlaytest();
	}

	// void updateVisibility(float p0) {
	// 	if (m_playbackMode == PlaybackMode::Playing) {
	// 		auto f = reinterpret_cast<MyGJBaseGameLayer*>(this)->m_fields.self();
	// 		if (f->m_enabled) {
	// 			float oldZoom = m_gameState.m_cameraZoom;
	// 			if (f->m_staticZoomEnabled) m_gameState.m_cameraZoom = f->m_staticZoom;
	// 			LevelEditorLayer::updateVisibility(p0);
	// 			m_gameState.m_cameraZoom = oldZoom;
	// 		} else {
	// 			LevelEditorLayer::updateVisibility(p0);
	// 		}
	// 	} else {
	// 		LevelEditorLayer::updateVisibility(p0);
	// 	}
	// }
};
