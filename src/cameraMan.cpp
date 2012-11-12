
#include "cameraMan.h"
#include <limits>

/**/
CameraMan::CameraMan(Ogre::Camera* cam) : 
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
}

CameraMan::~CameraMan() {}

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

bool CameraMan::frameRenderingQueued(const Ogre::FrameEvent& evt)
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
	PolyVox::RaycastResult resultX;
	PolyVox::RaycastResult resultY;
	PolyVox::RaycastResult resultZ;

	PolyVox::Vector3DFloat cameraPos( mCamera->getPosition().x, mCamera->getPosition().y, mCamera->getPosition().z );

	double width = 0.5;
	if( mVelocity.x > 0 )
	{
		mTerrain->raycast( cameraPos, PolyVox::Vector3DFloat( width, 0, 0), resultX);
	}
	else if( mVelocity.x < 0 )
	{
		mTerrain->raycast( cameraPos, PolyVox::Vector3DFloat(-width, 0, 0), resultX);
	}
	else
	{
		resultX.foundIntersection = false;
	}

	if( mVelocity.y > 0 )
	{
		mTerrain->raycast( cameraPos, PolyVox::Vector3DFloat( 0, 2*width, 0), resultY);
	}
	else if( mVelocity.y < 0 )
	{
		mTerrain->raycast( cameraPos, PolyVox::Vector3DFloat( 0,-2*width, 0), resultY);
	}
	else
	{
		resultY.foundIntersection = false;
	}

	if( mVelocity.z > 0 )
	{
		mTerrain->raycast( cameraPos, PolyVox::Vector3DFloat( 0, 0, width), resultZ);
	}
	else if( mVelocity.z < 0 )
	{
		mTerrain->raycast( cameraPos, PolyVox::Vector3DFloat( 0, 0,-width), resultZ);
	}
	else
	{
		resultZ.foundIntersection = false;
	}

	// handle horizontal movement
	if( resultX.foundIntersection )
	{
		mVelocity.x = 0;
	}
	if( resultZ.foundIntersection )
	{
		mVelocity.z = 0;
	}

	// handle ground/ceiling
	if( resultY.foundIntersection )
	{
		mVelocity.y = 0;
	}
	else
	{
		if( mVelocity.y < -fallSpeedMax )
		{
			mVelocity.y = -fallSpeedMax;
		}
	}

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
}

/*-----------------------------------------------------------------------------
  | Processes mouse movement differently for each style.
  -----------------------------------------------------------------------------*/
void CameraMan::injectMouseMove(const OIS::MouseEvent& evt)
{
	mCamera->yaw(Ogre::Degree(-evt.state.X.rel * 0.15f));
	mCamera->pitch(Ogre::Degree(-evt.state.Y.rel * 0.15f));
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
