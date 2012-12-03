
#include "cameraMan.h"
#include <limits>

/**/
CameraMan::CameraMan(Ogre::Camera* cam, std::string cameraName, uint32_t cameraId) : 
	PlayerEntity(cameraId, cameraName),
	mCamera(0)
	, mTopSpeed(150)
	, mVelocity(Ogre::Vector3::ZERO)
	, mGoingForward(false)
	, mGoingBack(false)
	, mGoingLeft(false)
	, mGoingRight(false)
	, mJump(false)
	, mFastMove(false)
{
	setCamera(cam);

	mCamera->setAutoTracking(false);
	mCamera->setFixedYawAxis(true);

	fallSpeedMax = 10.0;
	gravityAccel = 10.0;
	jumpSpeed = 5.0;
	charHeight = 2.0;
}

CameraMan::~CameraMan() {}


bool CameraMan::unserialize(Packet &packet)
{
	bool retVal = PlayerEntity::unserialize(packet);
	mCamera->setPosition(location);
	return retVal;
}

/*-----------------------------------------------------------------------------
  | Swaps the camera on our camera man for another camera.
  -----------------------------------------------------------------------------*/
void CameraMan::setCamera(Ogre::Camera* cam)
{
	mCamera = cam;
}

Ogre::Camera* CameraMan::getCamera()
{
	return mCamera;
}

/*-----------------------------------------------------------------------------
 * Set the terrain we're navigating in
 *-----------------------------------------------------------------------------*/
void CameraMan::setTerrain( TerrainPager *terrain )
{
	mTerrain = terrain;
}

/*-----------------------------------------------------------------------------
  | Sets the camera's top speed. Only applies for free-look style.
  -----------------------------------------------------------------------------*/
void CameraMan::setTopSpeed(Ogre::Real topSpeed)
{
	mTopSpeed = topSpeed;
}

Ogre::Real CameraMan::getTopSpeed()
{
	return mTopSpeed;
}

/*-----------------------------------------------------------------------------
  | Manually stops the camera when in free-look mode.
  -----------------------------------------------------------------------------*/
void CameraMan::manualStop()
{
	mGoingForward = false;
	mGoingBack = false;
	mGoingLeft = false;
	mGoingRight = false;
	mVelocity = Ogre::Vector3::ZERO;
}

Ogre::Vector3 CameraMan::calculateAccelerations()
{
	// build our acceleration vector based on keyboard input composite
	Ogre::Vector3 accel = Ogre::Vector3::ZERO;

	Ogre::Vector3 camDirection = mCamera->getDirection();
	camDirection.y = 0;
	camDirection.normalise();
	
	Ogre::Vector3 camRight = mCamera->getRight();
	camRight.y = 0;
	camRight.normalise();

	if (mGoingForward) accel += camDirection;
	if (mGoingBack) accel -= camDirection;
	if (mGoingRight) accel += camRight;
	if (mGoingLeft) accel -= camRight;

	return accel;
}

void CameraMan::sendMotionPacket()
{
	// update accelerations
	Ogre::Vector3 accel = calculateAccelerations();

	// Notify the server of the change
	//Packet modifyPacket;
	//modifyPacket.type = PLAYER_MOVE;

	// TODO: push accel of x, y, z
	//modifyPacket.push(vec.getX());
	//modifyPacket.push(vec.getY());
	//modifyPacket.push(vec.getZ());

	// TODO: send packet
	// modifyPacket.send( ClientApp::getInstance()->getServer(), ENET_PACKET_FLAG_RELIABLE );
}

void CameraMan::sendDirectionPacket()
{
	// Notify the server of the change
	//Packet modifyPacket;
	//modifyPacket.type = PLAYER_DIRECTION;

	// TODO: push mCamera->yaw, mCamera->pitch
	//modifyPacket.push(mCamera->yaw);
	//modifyPacket.push(mCamera->pitch);

	// TODO: send packet
	// modifyPacket.send( ClientApp::getInstance()->getServer(), ENET_PACKET_FLAG_RELIABLE );
}

bool CameraMan::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
	// update accelerations
	Ogre::Vector3 accel = calculateAccelerations();

	// if accelerating, try to reach top speed in a certain time
	Ogre::Real topSpeed = mFastMove ? mTopSpeed * 2 : mTopSpeed;
	double y_tmp = mVelocity.y;
	if (accel.squaredLength() != 0)
	{
		accel.normalise();
		mVelocity += accel * topSpeed * evt.timeSinceLastFrame * 10;
	}
	// if not accelerating, try to stop in a certain time
	else 
	{
		mVelocity -= mVelocity * evt.timeSinceLastFrame * 10;
	}
	mVelocity.y = y_tmp;

	// vertical movement
	if (mJump) mVelocity.y = jumpSpeed;
	mJump = false;
	mVelocity.y -= gravityAccel * evt.timeSinceLastFrame;

	Ogre::Real tooSmall = std::numeric_limits<Ogre::Real>::epsilon();

	y_tmp = mVelocity.y;
	mVelocity.y = 0;
	// keep camera velocity below top speed and above epsilon
	if (mVelocity.squaredLength() > topSpeed * topSpeed)
	{
		mVelocity.normalise();
		mVelocity *= topSpeed;
	}
	else if (mVelocity.squaredLength() < tooSmall * tooSmall)
	{
		mVelocity = Ogre::Vector3::ZERO;
	}
	mVelocity.y = y_tmp;

	// terrain handling, raycast each unit direction
	PolyVox::RaycastResult resultXH;
	PolyVox::RaycastResult resultXL;
	PolyVox::RaycastResult resultY;
	PolyVox::RaycastResult resultZH;
	PolyVox::RaycastResult resultZL;

	double width = 0.5;
	PolyVox::Vector3DFloat cameraPos( mCamera->getPosition().x, mCamera->getPosition().y,						mCamera->getPosition().z );
	PolyVox::Vector3DFloat cameraPosH( mCamera->getPosition().x, mCamera->getPosition().y + width,				mCamera->getPosition().z );
	PolyVox::Vector3DFloat cameraPosL( mCamera->getPosition().x, mCamera->getPosition().y - charHeight + width,	mCamera->getPosition().z );

	if( mVelocity.x > 0 )
	{
		mTerrain->raycast( cameraPosH, PolyVox::Vector3DFloat( width, 0, 0), resultXH);
		mTerrain->raycast( cameraPosL, PolyVox::Vector3DFloat( width, 0, 0), resultXL);
	}
	else if( mVelocity.x < 0 )
	{
		mTerrain->raycast( cameraPosH, PolyVox::Vector3DFloat(-width, 0, 0), resultXH);
		mTerrain->raycast( cameraPosL, PolyVox::Vector3DFloat(-width, 0, 0), resultXL);
	}
	else
	{
		resultXH.foundIntersection = false;
		resultXL.foundIntersection = false;
	}

	if( mVelocity.y > 0 )
	{
		mTerrain->raycast( cameraPosH, PolyVox::Vector3DFloat( 0, width, 0), resultY);
	}
	else if( mVelocity.y < 0 )
	{
		mTerrain->raycast( cameraPosL, PolyVox::Vector3DFloat( 0,-width, 0), resultY);
	}
	else
	{
		resultY.foundIntersection = false;
	}

	if( mVelocity.z > 0 )
	{
		mTerrain->raycast( cameraPosH, PolyVox::Vector3DFloat( 0, 0, width), resultZH);
		mTerrain->raycast( cameraPosL, PolyVox::Vector3DFloat( 0, 0, width), resultZL);
	}
	else if( mVelocity.z < 0 )
	{
		mTerrain->raycast( cameraPosH, PolyVox::Vector3DFloat( 0, 0,-width), resultZH);
		mTerrain->raycast( cameraPosL, PolyVox::Vector3DFloat( 0, 0,-width), resultZL);
	}
	else
	{
		resultZH.foundIntersection = false;
		resultZL.foundIntersection = false;
	}

	Ogre::Vector3 camPos( mCamera->getPosition() );
	// handle horizontal movement
	if( resultXH.foundIntersection || resultXL.foundIntersection )
	{
		mVelocity.x = 0;
		
		if( resultXH.foundIntersection )
		{
			camPos.x = resultXH.previousVoxel.getX();
		}
		else
		{
			camPos.x = resultXL.previousVoxel.getX();
		}
	}
	if( resultZH.foundIntersection || resultZL.foundIntersection )
	{
		mVelocity.z = 0;

		if( resultZH.foundIntersection )
		{
			camPos.z = resultZH.previousVoxel.getZ();
		}
		else
		{
			camPos.z = resultZL.previousVoxel.getZ();
		}
	}

	// handle ground/ceiling
	if( resultY.foundIntersection )
	{
		camPos.y = resultY.previousVoxel.getY();
		if( mVelocity.y < 0 )
		{
			camPos.y += charHeight - 1.0;
		}

		mVelocity.y = 0;
	}
	else
	{
		if( mVelocity.y < -fallSpeedMax )
		{
			mVelocity.y = -fallSpeedMax;
		}
	}

	// set new position, not in a voxel
	mCamera->setPosition( camPos );

	if (mVelocity != Ogre::Vector3::ZERO) mCamera->move(mVelocity * evt.timeSinceLastFrame);

	return true;
}

/*-----------------------------------------------------------------------------
  | Processes key presses for free-look style movement.
  -----------------------------------------------------------------------------*/
void CameraMan::injectKeyDown(const OIS::KeyEvent& evt)
{
	if (evt.key == OIS::KC_W || evt.key == OIS::KC_UP) mGoingForward = true;
	else if (evt.key == OIS::KC_S || evt.key == OIS::KC_DOWN) mGoingBack = true;
	else if (evt.key == OIS::KC_A || evt.key == OIS::KC_LEFT) mGoingLeft = true;
	else if (evt.key == OIS::KC_D || evt.key == OIS::KC_RIGHT) mGoingRight = true;
	else if (evt.key == OIS::KC_SPACE) mJump = true;
	else if (evt.key == OIS::KC_LSHIFT) mFastMove = true;

	sendMotionPacket();
}

/*-----------------------------------------------------------------------------
  | Processes key releases for free-look style movement.
  -----------------------------------------------------------------------------*/
void CameraMan::injectKeyUp(const OIS::KeyEvent& evt)
{
	if (evt.key == OIS::KC_W || evt.key == OIS::KC_UP) mGoingForward = false;
	else if (evt.key == OIS::KC_S || evt.key == OIS::KC_DOWN) mGoingBack = false;
	else if (evt.key == OIS::KC_A || evt.key == OIS::KC_LEFT) mGoingLeft = false;
	else if (evt.key == OIS::KC_D || evt.key == OIS::KC_RIGHT) mGoingRight = false;
	else if (evt.key == OIS::KC_SPACE) mJump = false;
	else if (evt.key == OIS::KC_LSHIFT) mFastMove = false;

	sendMotionPacket();
}

/*-----------------------------------------------------------------------------
  | Processes mouse movement differently for each style.
  -----------------------------------------------------------------------------*/
void CameraMan::injectMouseMove(const OIS::MouseEvent& evt)
{
	mCamera->yaw(Ogre::Degree(-evt.state.X.rel * 0.15f));
	mCamera->pitch(Ogre::Degree(-evt.state.Y.rel * 0.15f));

	sendDirectionPacket();
}

/*-----------------------------------------------------------------------------
  | Processes mouse presses. Only applies for orbit style.
  | Left button is for orbiting, and right button is for zooming.
  -----------------------------------------------------------------------------*/
void CameraMan::injectMouseDown(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
{
}

/*-----------------------------------------------------------------------------
  | Processes mouse releases. Only applies for orbit style.
  | Left button is for orbiting, and right button is for zooming.
  -----------------------------------------------------------------------------*/
void CameraMan::injectMouseUp(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
{
}
