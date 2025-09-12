# PlaytestCameraSettings
This is where she makes a mod.

<img src="logo.png" width="150" alt="the mod's logo" />

*Update logo.png to change your mod's icon (please)*

## Getting started
We recommend heading over to [the getting started section on our docs](https://docs.geode-sdk.org/getting-started/) for useful info on what to do next.

## Build instructions
For more info, see [our docs](https://docs.geode-sdk.org/getting-started/create-mod#build)
```sh
# Assuming you have the Geode CLI set up already
geode build
```

# Resources
* [Geode SDK Documentation](https://docs.geode-sdk.org/)
* [Geode SDK Source Code](https://github.com/geode-sdk/geode/)
* [Geode CLI](https://github.com/geode-sdk/cli)
* [Bindings](https://github.com/geode-sdk/bindings/)
* [Dev Tools](https://github.com/geode-sdk/DevTools)


// void updateZoom(float zoom, float duration, int easing, float rate, int uniqueID, int controlID) {
// 	char cVar1;
// 	PlayerObject *pPVar2;
// 	int iVar3;
// 	bool bVar4;
// 	float fVar5;
// 	float endZoom;
	
// 	if (zoom <= 0.0) {
// 	    zoom = 1.0;
// 	}
// 	fVar5 = 3.0;
// 	if (zoom <= 3.0) {
// 	    fVar5 = zoom;
// 	}
// 	endZoom = 0.4;
// 	if (0.4 <= fVar5) {
// 	    endZoom = fVar5;
// 	}
// 	m_gameState.m_targetCameraZoom = endZoom;
// 	if (duration <= 0.0) {
// 		m_gameState.stopTweenAction(0xe);
// 		m_gameState.m_cameraZoom = endZoom;
// 	}
// 	else {
// 		m_gameState.tweenValue(m_gameState.m_cameraZoom,endZoom,0xe,duration,easing,rate,uniqueID,controlID);
// 	}
// 	m_gameState.m_unkUint1 = duration;
// 	if ((m_skipArtReload == '\0') && (m_gameState.m_unkBool7 != '\0')) {
// 	    if ((false) || (duration != 0.0)) {
// 	        bVar4 = false;
// 	    }
// 	    else {
// 	        bVar4 = true;
// 	    }
// 		pPVar2 = m_player1;
// 	    if ((((pPVar2->m_isShip == false) &&
// 	         (pPVar2->m_isBird == false)) &&
// 	        (pPVar2->m_isDart == false)) &&
// 	       (pPVar2->m_isSwing == false)) {
// 	        if (pPVar2->m_isBall == false) {
// 	            iVar3 = 0x6;
// 	            if (pPVar2->m_isSpider != false) {
// 	                iVar3 = 0x21;
// 	            }
// 	        }
// 	        else {
// 	            iVar3 = 0x10;
// 	        }
// 	    }
// 	    else {
// 	        iVar3 = 0x5;
// 	    }
// 	    updateDualGround(pPVar2,iVar3,bVar4,duration);
// 	}
// 	return;

// }

