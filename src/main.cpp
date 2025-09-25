#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorUI.hpp>

#ifdef GEODE_IS_WINDOWS
	#include <Geode/modify/CCEGLView.hpp>
#endif

#include <numbers>

/*
! Notes:
scenePoint = edPos + edPoint * edScale
edPoint = (scenePoint - edPos) / edScale
edPos = scenePoint - edPoint * edScale     <-- position of m_objectLayer

m_gameState.m_cameraPosition, m_gameState.m_cameraPosition2 - bottom left camera corner in editor coords

*/

inline CCPoint scenePoint(CCPoint edPoint, CCPoint edPos, float edScale) {
	return edPos + edPoint * edScale;
}

inline CCPoint editorPoint(CCPoint scenePoint, CCPoint edPos, float edScale) {
	return (scenePoint - edPos) / edScale;
}

inline CCPoint edPos(CCPoint edPoint, CCPoint scenePoint, float edScale) {
	return scenePoint - edPoint * edScale;
}


class $modify(MyGJBaseGameLayer, GJBaseGameLayer) {

	struct Fields {
		bool m_enabled{};
		bool m_staticCameraEnabled{};
		bool m_staticZoomEnabled{};
		bool m_staticRotEnabled{};
		bool m_debugDrawEnabled{};

		// desired camera center position and zoom (in Editor coords)
		CCPoint m_staticCenterPos{};
		float m_staticZoom{};

		// m_objectLayer position
		CCPoint m_lastPos{};
		float m_lastScale{1.f};

		struct {
			CCPoint m_oldGameStateCameraPos2{};
			float m_oldGameStateZoom{1.f};
			float m_oldGameStateCameraAngle{};
		} m_originalValues;

		Ref<CCDrawNode> m_drawWinRectNode;

		struct {
			bool m_shouldInsertCameraUpdate{};
			bool m_enableFix{};
		} m_startposCameraFix;
	};


	void setupModDebugDrawNode() {
		m_fields->m_drawWinRectNode = CCDrawNode::create();
		m_fields->m_drawWinRectNode->setID("debug-draw-node"_spr);
		m_debugDrawNode->addChild(m_fields->m_drawWinRectNode, 3000);
	}


	$override
	void visit() {
		auto f = m_fields.self();
		if (f->m_enabled) {
			bool ground = m_groundLayer->isVisible();
			bool ground2 = m_groundLayer2->isVisible();
			bool mg = m_middleground ? m_middleground->isVisible() : false;

			m_groundLayer->setVisible(false);
			m_groundLayer2->setVisible(false);
			if (m_middleground) m_middleground->setVisible(false);

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
			if (m_middleground) m_middleground->setVisible(mg);

		} else {
			GJBaseGameLayer::visit();
		}
	}


	inline void setScalePosAllLayers(CCPoint pos, float scale) {
		auto debugDrawLayer = m_debugDrawNode->getParent();
		debugDrawLayer->setScale(scale);
		debugDrawLayer->setPosition(pos);

		m_objectLayer->setScale(scale);
		m_objectLayer->setPosition(pos);

		m_inShaderObjectLayer->setPosition(pos);
		m_inShaderObjectLayer->setScale(scale);

		m_aboveShaderObjectLayer->setPosition(pos);
		m_aboveShaderObjectLayer->setScale(scale);
	}


	void playtestCameraUpdate() {
		auto f = m_fields.self();
		if (!f->m_enabled) return;
		
		auto hws = CCDirector::get()->getWinSize() / 2;
		auto scaledSz = hws / m_gameState.m_cameraZoom;
		auto camCenterInEditor = m_gameState.m_cameraPosition + scaledSz;
				
		if (f->m_debugDrawEnabled) {
			float angleRad = m_gameState.m_cameraAngle / 180.f * std::numbers::pi;
			float cosA = std::cos(angleRad);
			float sinA = std::sin(angleRad);
		
			float hw = scaledSz.width;
			float hh = scaledSz.height;
			CCPoint points[] = {{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}};
		
			for (int i = 0; i < 4; ++i) {
				float xr = points[i].x * cosA - points[i].y * sinA;
				float yr = points[i].x * sinA + points[i].y * cosA;
				points[i] = camCenterInEditor + ccp(xr, yr);
			}
		
			f->m_drawWinRectNode->clear();
			f->m_drawWinRectNode->drawPolygon(points, 4, ccc4f(0,0,0,0), 1.5, ccc4f(1,1,0,1));
		}

		auto zoom = f->m_staticZoomEnabled ? f->m_staticZoom : m_gameState.m_cameraZoom;
		auto center = f->m_staticCameraEnabled ? f->m_staticCenterPos : camCenterInEditor;

		auto newEdPos = edPos(center, hws, zoom);
		setScalePosAllLayers(newEdPos, zoom);

		f->m_lastPos = newEdPos;
		f->m_lastScale = zoom;
	}


	$override
	void update(float p0) {
		auto f = m_fields.self();
		if (f->m_enabled) {
			CCPoint oldBgPos = m_background->getPosition();
			float oldBgZoom = m_background->getScale();

			// это нужно вызывать со всеми оригинальными значениями
			GJBaseGameLayer::update(p0);
			playtestCameraUpdate();
			
			if (f->m_staticCameraEnabled) m_background->setPosition(oldBgPos);
			if (f->m_staticZoomEnabled) m_background->setScale(oldBgZoom);
		} else {
			GJBaseGameLayer::update(p0);
		}
	}


	void restoreStaticCamera() {
		auto f = m_fields.self();
		if (!f->m_enabled) return;
		auto hws = CCDirector::get()->getWinSize() / 2;
		auto edPoint = editorPoint(hws, f->m_lastPos, f->m_lastScale);
		auto nextZoom = f->m_staticZoomEnabled ? f->m_lastScale : m_objectLayer->getScale();
		auto p = edPos(edPoint, hws, nextZoom);
		setScalePosAllLayers(p, nextZoom);
	}


	void restoreStaticCamera(CCPoint p, float zoom) {
		auto f = m_fields.self();
		if (!f->m_enabled) return;
		setScalePosAllLayers(p, zoom);
	}


	$override
	void updateDebugDraw() {
		auto f = m_fields.self();
		if (f->m_enabled) {
			CCPoint oldPos = m_objectLayer->getPosition();
			float oldZoom = m_objectLayer->getScale();
			m_objectLayer->setPosition(f->m_lastPos);
			m_objectLayer->setScale(f->m_lastScale);
			// это нужно вызывать с моими значениями у m_objectLayer
			GJBaseGameLayer::updateDebugDraw();
			m_objectLayer->setPosition(oldPos);
			m_objectLayer->setScale(oldZoom);
		} else {
			GJBaseGameLayer::updateDebugDraw();
		}
	}

	
	$override
	void processAreaEffects(gd::vector<EnterEffectInstance>* p0, GJAreaActionType p1, float p2, bool p3) {
		auto f = m_fields.self();
		if (f->m_enabled) {
			auto oldZoom = m_gameState.m_cameraZoom;
			auto oldAngle = m_gameState.m_cameraAngle;
			auto oldPos2 = m_gameState.m_cameraPosition2;
			m_gameState.m_cameraZoom = f->m_originalValues.m_oldGameStateZoom;
			m_gameState.m_cameraAngle = f->m_originalValues.m_oldGameStateCameraAngle;
			m_gameState.m_cameraPosition2 = f->m_originalValues.m_oldGameStateCameraPos2;
			// это нужно вызывать с ориг. значениями у m_cameraZoom, m_cameraAngle, m_cameraPosition2
			GJBaseGameLayer::processAreaEffects(p0, p1, p2, p3);
			m_gameState.m_cameraZoom = oldZoom;
			m_gameState.m_cameraAngle = oldAngle;
			m_gameState.m_cameraPosition2 = oldPos2;
		} else {
			GJBaseGameLayer::processAreaEffects(p0, p1, p2, p3);
		}
	}

	// -------------------------- RobTop's camera bug fix --------------------------

	void loadStartPosObject() {
		auto f = m_fields.self();
		f->m_startposCameraFix.m_shouldInsertCameraUpdate = f->m_startposCameraFix.m_enableFix;
		GJBaseGameLayer::loadStartPosObject();
		f->m_startposCameraFix.m_shouldInsertCameraUpdate = false;
	}

	void processMoveActionsStep(float p0, bool p1) {
		GJBaseGameLayer::processMoveActionsStep(p0, p1);
		if (m_fields->m_startposCameraFix.m_shouldInsertCameraUpdate) {
			// yoinked this piece of code from GJBaseGameLayer::update() - addr: 0x233030
			m_gameState.processStateTriggers();
			m_gameState.updateTweenActions(p0);
			updateCamera(p0);
		}
	}
};


class $modify(LevelEditorLayer) {

	$override
	bool init(GJGameLevel* p0, bool p1) {
		if (!LevelEditorLayer::init(p0, p1)) return false;
		reinterpret_cast<MyGJBaseGameLayer*>(this)->setupModDebugDrawNode();
		return true;
	}


	$override
	void updateVisibility(float p0) {
		auto f = reinterpret_cast<MyGJBaseGameLayer*>(this)->m_fields.self();
		if (f->m_enabled) {

			f->m_originalValues.m_oldGameStateZoom = m_gameState.m_cameraZoom;
			f->m_originalValues.m_oldGameStateCameraAngle = m_gameState.m_cameraAngle;
			f->m_originalValues.m_oldGameStateCameraPos2 = m_gameState.m_cameraPosition2;

			if (f->m_staticZoomEnabled) m_gameState.m_cameraZoom = f->m_staticZoom;
			if (f->m_staticRotEnabled) m_gameState.m_cameraAngle = 0;

			auto hws = CCDirector::get()->getWinSize() / 2;

			if (f->m_staticCameraEnabled) {
				auto winCenterInEditorScale = hws / m_gameState.m_cameraZoom;
				m_gameState.m_cameraPosition2 = f->m_staticCenterPos - winCenterInEditorScale;
			} else {
				auto diffInEditorCoords = hws * (1 / f->m_originalValues.m_oldGameStateZoom - 1 / m_gameState.m_cameraZoom);
				m_gameState.m_cameraPosition2 = m_gameState.m_cameraPosition2 + diffInEditorCoords;
			}

			// это нужно вызывать с моими значениями у m_cameraZoom, m_cameraAngle, m_cameraPosition2
			LevelEditorLayer::updateVisibility(p0);

			m_gameState.m_cameraZoom = f->m_originalValues.m_oldGameStateZoom;
			m_gameState.m_cameraAngle = f->m_originalValues.m_oldGameStateCameraAngle;
			m_gameState.m_cameraPosition2 = f->m_originalValues.m_oldGameStateCameraPos2;

		} else {
			LevelEditorLayer::updateVisibility(p0);
		}
	}

	void updatePlaytestValues() {
		auto zoom = m_objectLayer->getScale();
		auto hws = CCDirector::get()->getWinSize() / 2;
		auto camCenter = editorPoint(hws, m_objectLayer->getPosition(), zoom);
		
		auto f = reinterpret_cast<MyGJBaseGameLayer*>(this)->m_fields.self();
		
		f->m_enabled = Mod::get()->getSavedValue<bool>("enabled", true);
		f->m_staticCameraEnabled = Mod::get()->getSavedValue<bool>("static_pos", true);
		f->m_staticZoomEnabled = Mod::get()->getSavedValue<bool>("static_zoom", true);
		f->m_staticRotEnabled = Mod::get()->getSavedValue<bool>("static_rot", true);
		f->m_debugDrawEnabled = Mod::get()->getSavedValue<bool>("window_rect", true);
		
		f->m_staticCenterPos = camCenter;
		f->m_staticZoom = zoom;

		f->m_startposCameraFix.m_enableFix = Mod::get()->getSettingValue<bool>("fix-startpos-camera-bug");
	}
	
	
	void destroyPlaytestValues() {
		auto f = reinterpret_cast<MyGJBaseGameLayer*>(this)->m_fields.self();
		f->m_enabled = false;
		if (f->m_drawWinRectNode) f->m_drawWinRectNode->clear();
	}


	$override
	void onStopPlaytest() {
		auto mode = m_playbackMode;
		auto pos = m_objectLayer->getPosition();
		auto scale = m_objectLayer->getScale();
		LevelEditorLayer::onStopPlaytest();
		
		if (mode == PlaybackMode::Paused) {
			queueInMainThread([this, pos, scale] {
				reinterpret_cast<MyGJBaseGameLayer*>(this)->restoreStaticCamera(pos, scale);
				destroyPlaytestValues();
			});
		} else if (mode == PlaybackMode::Playing) {
			queueInMainThread([this] {
				reinterpret_cast<MyGJBaseGameLayer*>(this)->restoreStaticCamera();
				destroyPlaytestValues();
			});
		} else {
			destroyPlaytestValues();
		}
	}


	$override
	void onPlaytest() {
		updatePlaytestValues();
		LevelEditorLayer::onPlaytest();
	}


	$override
	void onResumePlaytest() {
		updatePlaytestValues();
		LevelEditorLayer::onResumePlaytest();
	}
};


#ifdef GEODE_IS_DESKTOP

class $modify(MyEditorUI, EditorUI) {

	static void onModify(auto& self) {
		if (!self.setHookPriority("EditorUI::scrollWheel", Priority::VeryEarly)) {
			log::warn("Failed to set hook priority");
		}
	}


	$override
	void scrollWheel(float y, float x) { 
		auto f = reinterpret_cast<MyGJBaseGameLayer*>(m_editorLayer)->m_fields.self();

		if (!f->m_enabled || m_editorLayer->m_playbackMode != PlaybackMode::Playing) {
			EditorUI::scrollWheel(y, x);
			return;
		}
		
		// Scroll during playtesting (used Better Edit code to be consistent)
		if (CCKeyboardDispatcher::get()->getControlKeyPressed()) {
			if (f->m_staticZoomEnabled) {
				float zoom = std::pow(std::numbers::e, std::log(std::max(f->m_staticZoom, .001f)) - y * .01f);
				zoom = std::clamp(zoom, .1f, 10000000.f);
	
				if (f->m_staticCameraEnabled) {
					auto mousePoint = getMousePos();
					auto hws = CCDirector::get()->getWinSize() / 2;
	
					auto objLayerPos = edPos(f->m_staticCenterPos, hws, f->m_staticZoom);
					auto mousePointInEditor = editorPoint(mousePoint, objLayerPos, f->m_staticZoom);
	
					auto newObjLayerPos = edPos(mousePointInEditor, mousePoint, zoom);
					f->m_staticCenterPos = editorPoint(hws, newObjLayerPos, zoom);
				}
				f->m_staticZoom = zoom;
			}
		} else { // otherwise move screen
			if (f->m_staticCameraEnabled) {
				auto zoom = f->m_staticZoomEnabled ? f->m_staticZoom : m_editorLayer->m_objectLayer->getScale();
				float diff = -y * 2 / zoom;

				if (CCKeyboardDispatcher::get()->getShiftKeyPressed()) {
					f->m_staticCenterPos.x -= diff;
				} else {
					#ifdef GEODE_IS_MACOS
						f->m_staticCenterPos.x += diff;
					#else
						f->m_staticCenterPos.y += diff;
					#endif
				}
			}
		}
	}


	void onMiddleClickPanning(CCPoint diff) {
		auto f = reinterpret_cast<MyGJBaseGameLayer*>(m_editorLayer)->m_fields.self();
		if (!f->m_enabled || m_editorLayer->m_playbackMode != PlaybackMode::Playing) {
			return;
		}
		if (f->m_staticCameraEnabled) {
			float zoom = f->m_staticZoomEnabled ? f->m_staticZoom : m_editorLayer->m_gameState.m_cameraZoom;
			f->m_staticCenterPos += diff / zoom;
		}
	}
};

#ifdef GEODE_IS_WINDOWS

bool isPanning{};
CCPoint lastClick{};

class $modify(CCEGLView) {
    $override 
	void onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int mods) {
		CCEGLView::onGLFWMouseCallBack(window, button, action, mods);
		if (button != GLFW_MOUSE_BUTTON_MIDDLE) return;

		if (action == GLFW_PRESS) {
			isPanning = true;
			lastClick = getMousePos();
		} else {
			isPanning = false;
		}
	}


    $override 
	void onGLFWMouseMoveCallBack(GLFWwindow* window, double x, double y) {
		CCEGLView::onGLFWMouseMoveCallBack(window, x, y);
		if (!isPanning) return;
		if (auto editor = EditorUI::get()) {
			auto pos = getMousePos();
			reinterpret_cast<MyEditorUI*>(editor)->onMiddleClickPanning(lastClick - pos);
			lastClick = pos;
		}
	}
};

#endif

#endif
