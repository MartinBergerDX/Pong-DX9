/* Pong 
   Left-handed coordinate system, camera looks down from negative Z axis.
      Y
   -X z X   // z is +/-Z
     -Y
*/

#include "d3dApp.h"
#include "DirectInput.h"
#include <crtdbg.h>
#include "GfxStats.h"
#include <list>
#include <time.h> // time(NULL)
#include <tchar.h> // _T, _tcscpy


static float r_angle;

class BallInfo
{
public:
   BallInfo() : BALL_SPEED(300), BALL_MAX_SPEED(1000), BALL_ACCEL(200), BALL_DRAG(100) { }
	D3DXVECTOR3 pos;
   float speed;
	float rotation;
	float life;

	const float BALL_SPEED;
	const float BALL_MAX_SPEED;
	const float BALL_ACCEL;
	const float BALL_DRAG; 
};


class PadInfo
{
public:
   PadInfo() : PAD_SPEED(300.0f)
   {
      pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f); 
      center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
      rotation = 0.0f; 
      energy = 0.0f;
   }

   D3DXVECTOR3 pos;
   D3DXVECTOR3 center;

   float rotation;         // rotation of pad image in radians.
   float energy;           // not yet implemented
   const float PAD_SPEED;  // speed

public:
   D3DXVECTOR3 bound1; // set upper-right rectangle corner
   D3DXVECTOR3 bound2; // set lower-left rectangle corner

   // input: bounding increment.
   //        increment must be positive value.
   // pre:   pos.x and pos.y must be set to valid 2d position.
   // post:  boundNW and boundSE are set to rectangle enclosing pad object.
   void setBoundingBox(const D3DXVECTOR3& inc1)
   {
      bound1.x = pos.x - inc1.x; bound1.y = pos.y + inc1.y; bound1.z = 0.0f;
      bound2.x = pos.x + inc1.x; bound2.y = pos.y - inc1.y; bound2.z = 0.0f;
   }

   bool checkCollision(const D3DXVECTOR3& object)
   {
      if (object.x > bound1.x && object.y < bound1.y &&
          object.x < bound2.x && object.y > bound2.y)
         return true;

      return false;
   }

   void updateBoundingBox(const float& incr)
   {
      bound1.y += incr;
      bound2.y += incr;
   }
};


class PongDemo : public D3DApp
{
public:
	PongDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~PongDemo();

	bool checkDeviceCaps();
	void onLostDevice();
	void onResetDevice();
	void updateScene(float dt);
	void drawScene();

	// Helper functions.
	void updateBall(float dt);
   void updatePad(float dt);
   void updateCamera(float dt); // update Z axis
	void drawBkgd();
	void drawPad();
	void drawBall();
   void drawScore();

private:
	GfxStats* mGfxStats;
	
	ID3DXSprite* mSprite; // http://msdn.microsoft.com/en-us/library/windows/desktop/bb174249%28v=vs.85%29.aspx
   ID3DXLine*   mLine;
   ID3DXFont*   mFont;

   float mCameraPosZ;
   RECT field;
   int player1Score;
   int player2Score;

	IDirect3DTexture9* mBkgdTex;
	D3DXVECTOR3 mBkgdCenter;

	IDirect3DTexture9* mBallTex;
	D3DXVECTOR3 mBallCenter;

	IDirect3DTexture9* mPadTex;

public:
   BallInfo ball;
   PadInfo pad1;
   PadInfo pad2;
};


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
	#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif

	PongDemo app(hInstance, "PongDemo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE | DISCL_FOREGROUND, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	gDInput = &di;

	return gd3dApp->run();
}

PongDemo::PongDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

   // seed random number generator
   srand((unsigned int) time(NULL));
   // create contained GfxStats dynamic object:
   mGfxStats = new GfxStats();

   // line:
   HR(D3DXCreateLine(gd3dDevice, &mLine));
   // sprite:
	HR(D3DXCreateSprite(gd3dDevice, &mSprite));
   // font:
	D3DXFONT_DESC fontDesc;
	fontDesc.Height          = 18;
   fontDesc.Width           = 0;
   fontDesc.Weight          = 0;
   fontDesc.MipLevels       = 1;
   fontDesc.Italic          = false;
   fontDesc.CharSet         = DEFAULT_CHARSET;
   fontDesc.OutputPrecision = OUT_DEFAULT_PRECIS;
   fontDesc.Quality         = DEFAULT_QUALITY;
   fontDesc.PitchAndFamily  = DEFAULT_PITCH | FF_DONTCARE;
#pragma warning(disable: 4996)
   _tcscpy(fontDesc.FaceName, _T("Times New Roman"));
#pragma warning(default: 4996)
	HR(D3DXCreateFontIndirect(gd3dDevice, &fontDesc, &mFont));

   // set camera height:
   mCameraPosZ = -1000.f;
   // initialize player scores:
   player1Score = 0; player2Score = 0;

   // load textures:
	HR(D3DXCreateTextureFromFile(gd3dDevice, "bkgd1.bmp", &mBkgdTex));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "ball.bmp",  &mBallTex));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "pad.bmp",   &mPadTex));

   // set background data:
	mBkgdCenter = D3DXVECTOR3(256.0f, 256.0f, 0.0f);

   // set field dimensions:
   /*RECT R;
   GetClientRect(mhMainWnd, &R);*/
   field.top    =  400; //   R.bottom / 2; 
   field.bottom = -400; // -(R.bottom / 2); 
   field.right  =  550; //   R.right / 2; 
   field.left   = -550; // -(R.right / 2);

   // set ball data:
	mBallCenter = D3DXVECTOR3(32.0f, 32.0f, 0.0f);
   ball.pos.x = 0.0f; ball.pos.y = 0.0f; ball.pos.z = 0;
   ball.rotation = D3DXToRadian(200);
   r_angle = ball.rotation;

   // set pads data:
   pad1.center = D3DXVECTOR3(64.0f, 64.0f, 0.0f);
	pad2.center = D3DXVECTOR3(64.0f, 64.0f, 0.0f);
   pad1.pos.x = (float) field.left;  pad1.pos.y = 0.0f; pad1.pos.z = 0.0f;
   pad2.pos.x = (float) field.right; pad2.pos.y = 0.0f; pad2.pos.z = 0.0f;
   pad2.rotation = D3DX_PI;
   pad1.setBoundingBox(D3DXVECTOR3(50.0f, 70.0f, 0.0f));
   pad2.setBoundingBox(D3DXVECTOR3(50.0f, 70.0f, 0.0f));

	onResetDevice();
}

PongDemo::~PongDemo()
{
	delete mGfxStats;
	ReleaseCOM(mSprite);
   ReleaseCOM(mLine);
   ReleaseCOM(mFont);
	ReleaseCOM(mBkgdTex);
	ReleaseCOM(mBallTex);
	ReleaseCOM(mPadTex);
}

bool PongDemo::checkDeviceCaps()
{
	// Nothing to check.
	return true;
}

void PongDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	HR(mSprite->OnLostDevice());
   HR(mLine->OnLostDevice());
   HR(mFont->OnLostDevice());
}

void PongDemo::onResetDevice()
{
	// Call the onResetDevice of other objects.
	mGfxStats->onResetDevice();
	HR(mSprite->OnResetDevice());
   HR(mLine->OnResetDevice());
   HR(mFont->OnResetDevice());

	// Sets up the camera 1000 units back looking at the origin.
	D3DXMATRIX V;
	D3DXVECTOR3 pos(0.0f, 0.0f, mCameraPosZ); //-1000.0f
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXMatrixLookAtLH(&V, &pos, &target, &up);
	HR(gd3dDevice->SetTransform(D3DTS_VIEW, &V));

	// The following code defines the volume of space the camera sees.
	D3DXMATRIX P;
	RECT R;
	GetClientRect(mhMainWnd, &R);
	float width  = (float)R.right;
	float height = (float)R.bottom;
	D3DXMatrixPerspectiveFovLH(&P, D3DX_PI*0.25f, width/height, 1.0f, 5000.0f);
	HR(gd3dDevice->SetTransform(D3DTS_PROJECTION, &P));

	// This code sets texture filters, which helps to smooth out distortions
	// when you scale a texture.  
	HR(gd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
	HR(gd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
	HR(gd3dDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR));

	// This line of code disables Direct3D lighting.
	HR(gd3dDevice->SetRenderState(D3DRS_LIGHTING, false));
	
	// The following code specifies an alpha test and reference value.
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHAREF, 10));
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER));

	// The following code is used to setup alpha blending.
	HR(gd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE));
	HR(gd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1));
	HR(gd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
	HR(gd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));

	// Indicates that we are using 2D texture coordinates.
	HR(gd3dDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2));
}

void PongDemo::updateScene(float dt)
{
	// Two triangles for each sprite--two for background,
	// two for ship, and two for each bullet.  Similarly,
	// 4 vertices for each sprite.
	/*mGfxStats->setTriCount(0);
	mGfxStats->setVertexCount(0);
	mGfxStats->update(dt);*/

	// Get snapshot of input devices.
	gDInput->poll();

	// Update game objects.
   updateCamera(dt);
	updateBall(dt);
   updatePad(dt);
}

void PongDemo::updateBall(float dt)
{
   if(gDInput->keyDown(DIK_R))
   {
      ball.rotation = r_angle;
      ball.pos.x = ball.pos.y = ball.pos.z = 0.0f;
   }
   if(gDInput->keyDown(DIK_T))
      ball.rotation -= D3DX_PI * dt;//D3DXToRadian(1);
   if(gDInput->keyDown(DIK_G))
      ball.rotation += D3DX_PI * dt;

   // check upper and lower bounds for collision, upper first:
   if (ball.pos.y > field.top)
   {
      if (ball.rotation >= D3DX_PI)
      {
         ball.rotation = D3DX_PI + (2 * D3DX_PI - ball.rotation);
      }
      else 
         if (ball.rotation < D3DX_PI)
         {
            ball.rotation += D3DX_PI - ball.rotation * 2;
         }
      ball.pos.y -= 10;
   }
   // now check for lower bound:
   else if (ball.pos.y < field.bottom)
   {
      if (ball.rotation <= D3DX_PI)
      {
         ball.rotation = D3DX_PI - ball.rotation;
      }
      else if (ball.rotation > D3DX_PI)
      {
         float temp = ball.rotation - D3DX_PI;
         ball.rotation = (2 * D3DX_PI) - temp;
      }
      ball.pos.y += 10;
   }

   // check for left collision:
   if (ball.pos.x < field.left)
   {
      //ball.rotation = 2 * D3DX_PI - ball.rotation;
      player2Score++;
      //Sleep(500);
      ball.pos.x = ball.pos.y = 0;
      int angleDegree = rand() % 360;
      ball.rotation = D3DXToRadian(angleDegree);
   }
   else if (ball.pos.x > field.right)
   {
      player1Score++;
      //Sleep(500);
      ball.pos.x = ball.pos.y = 0;
      int angleDegree = rand() % 360;
      ball.rotation = D3DXToRadian(angleDegree);
   }

   // check for pads collision:
   if (pad1.checkCollision(ball.pos))
   {
      ball.rotation = (2 * D3DX_PI) - ball.rotation;
      ball.pos.x += 20;
      //debugging:
      /*ball.pos.x = ball.pos.y = 0.0f;
      ball.rotation += D3DXToRadian(5.0f);*/
   }
   if (pad2.checkCollision(ball.pos))
   {
      ball.rotation = ((2 * D3DX_PI) - ball.rotation);
      ball.pos.x -= 20;
   }

	D3DXVECTOR3 dir(-sinf(ball.rotation), cosf(ball.rotation), 0.0f);
   ball.pos += dir * ball.BALL_SPEED * dt;
}

void PongDemo::updatePad(float dt)
{
   // Pad1:
	// Check input.
   if(gDInput->keyDown(DIK_W))	
   {
      if (pad1.pos.y < field.top)
      {
         float old = pad1.pos.y;              // store old value
         pad1.pos.y += pad1.PAD_SPEED * dt;   // increment pad
         float difference = pad1.pos.y - old; // incremented value - old value
         pad1.updateBoundingBox(difference);  // update bounding box
      }
   }
   if(gDInput->keyDown(DIK_S))
   {
      if (pad1.pos.y > field.bottom)
      {
         float old = pad1.pos.y;             // store old value
         pad1.pos.y -= pad1.PAD_SPEED * dt;  // decrement pad
         float difference = abs(abs(pad1.pos.y) - abs(old));  // decremented value - old value
         pad1.updateBoundingBox(-difference);   // update bounding box
      }
   }

   // Pad2:
	// Check input.
   if(gDInput->keyDown(DIK_NUMPAD8))
   {
      if (pad2.pos.y < field.top)
      {
         float old = pad2.pos.y;              // store old value
         pad2.pos.y += pad2.PAD_SPEED * dt;   // increment pad
         float difference = pad2.pos.y - old; // incremented value - old value
         pad2.updateBoundingBox(difference);  // update bounding box
      }
   }
	if(gDInput->keyDown(DIK_NUMPAD5))
   {
      if (pad2.pos.y > field.bottom)
      {
         float old = pad2.pos.y;              // store old value
         pad2.pos.y -= pad2.PAD_SPEED * dt;   // decrement pad
         float difference = abs(abs(pad2.pos.y) - abs(old));  // incremented value - old value
         pad2.updateBoundingBox(-difference); // update bounding box
      }
   }
}

void PongDemo::updateCamera(float dt)
{
   float z = gDInput->mouseDZ();
   mCameraPosZ += z;

	D3DXMATRIX V;
	D3DXVECTOR3 pos(0.0f, 0.0f, mCameraPosZ); //-1000.0f
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXMatrixLookAtLH(&V, &pos, &target, &up);
	HR(gd3dDevice->SetTransform(D3DTS_VIEW, &V));
}

void PongDemo::drawScene()
{
	// Clear the backbuffer and depth buffer.
	HR(gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0));
	HR(gd3dDevice->BeginScene());

   // draw lines:
   D3DCOLOR color_green = D3DCOLOR_XRGB(57, 251, 36);
   /*const DWORD vectorsSize = 2; //sizeof(vectors) / sizeof(vectors[0])
   D3DXVECTOR2 vectors1[vectorsSize] = {D3DXVECTOR2(50.0f, 50.0f), D3DXVECTOR2(400.0f, 50.0f)};
   D3DXVECTOR2 vectors2[vectorsSize] = {D3DXVECTOR2(0.0f, 50.0f), D3DXVECTOR2(800.0f, 50.0f)};*/
   /*const DWORD vectorsSize3 = 4;
   D3DXVECTOR2 vectors3[vectorsSize3] = {D3DXVECTOR2(pad1.pos.x, pad1.pos.y), D3DXVECTOR2(pad2.pos.x, pad1.pos.y), 
                                         D3DXVECTOR2(pad2.pos.x, pad2.pos.y), D3DXVECTOR2(pad1.pos.x, pad2.pos.y)};*/
   /*HR(mLine->Begin());
   HR(mLine->Draw(vectors3, vectorsSize3, color_green));
   HR(mLine->End());*/
   // end draw lines.

	HR(mSprite->Begin(D3DXSPRITE_OBJECTSPACE | D3DXSPRITE_DONOTMODIFY_RENDERSTATE));
	drawBkgd();
	drawPad();	
	drawBall();
	//mGfxStats->display();
   drawScore();

	HR(mSprite->End());
	HR(gd3dDevice->EndScene());
	// Present the backbuffer.
	HR(gd3dDevice->Present(0, 0, 0, 0));
}

void PongDemo::drawBkgd()
{
	// Set a texture coordinate scaling transform.  Here we scale the texture 
	// coordinates by 10 in each dimension. This tiles the texture 
	// ten times over the sprite surface.
	D3DXMATRIX texScaling;
	D3DXMatrixScaling(&texScaling, 10.0f, 10.0f, 0.0f);
	HR(gd3dDevice->SetTransform(D3DTS_TEXTURE0, &texScaling));

	D3DXMATRIX T, S;
	D3DXMatrixScaling(&S, 20.0f, 20.0f, 0.0f);
	HR(mSprite->SetTransform(&S));

	// Draw the background sprite.
	HR(mSprite->Draw(mBkgdTex, 0, &mBkgdCenter, 0, D3DCOLOR_XRGB(255, 255, 255)));
	HR(mSprite->Flush());

	// Restore defaults texture coordinate scaling transform.
	D3DXMatrixScaling(&texScaling, 1.0f, -1.0f, 0.0f);
	HR(gd3dDevice->SetTransform(D3DTS_TEXTURE0, &texScaling));
}

void PongDemo::drawPad()
{
	// Turn on the alpha test.
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, true));

	// Set orientation.
	D3DXMATRIX T, R;
   
   D3DXMatrixTranslation(&T, pad1.pos.x, pad1.pos.y, pad1.pos.z);
	HR(mSprite->SetTransform(&T));
   HR(mSprite->Draw(mPadTex, 0, &pad1.center, 0, D3DCOLOR_XRGB(255, 255, 255)));
	
   D3DXMatrixTranslation(&T, pad2.pos.x, pad2.pos.y, pad2.pos.z);
   D3DXMatrixRotationZ(&R, pad2.rotation);
   HR(mSprite->SetTransform(&(R*T)));
   HR(mSprite->Draw(mPadTex, 0, &pad2.center, 0, D3DCOLOR_XRGB(255, 255, 255)));
   
   HR(mSprite->Flush());

	// Turn off the alpha test.
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, false));
}

void PongDemo::drawBall()
{
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true));
	D3DXMATRIX T;
	D3DXMatrixTranslation(&T, ball.pos.x, ball.pos.y, ball.pos.z);
	HR(mSprite->SetTransform(&T));
	HR(mSprite->Draw(mBallTex, 0, &mBallCenter, 0, D3DCOLOR_XRGB(255, 255, 255)));
	HR(mSprite->Flush());
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false));
}

void PongDemo::drawScore()
{
	// Make static so memory is not allocated every frame.
	static char buffer[256];
#pragma warning(disable: 4996)
	sprintf(buffer, "Player 1 score:  %d\n"
                   "Player 2 score:  %d", player1Score, player2Score);
#pragma warning(default: 4996)
	RECT R = {5, 5, 0, 0};
	HR(mFont->DrawText(0, buffer, -1, &R, DT_NOCLIP, D3DCOLOR_XRGB(0,0,0)));
}