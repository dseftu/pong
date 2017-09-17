#include "pch.h"
#include "PongGame.h"
#include "Ball.h"
#include "Paddle.h"

using namespace std;
using namespace DirectX;
using namespace Library;
using namespace Microsoft::WRL;

namespace Pong
{
	const XMVECTORF32 PongGame::BackgroundColor = Colors::Black;
	const int MAXSCORE = 3;

	PongGame::PongGame(function<void*()> getWindowCallback, function<void(SIZE&)> getRenderTargetSizeCallback) :
		Game(getWindowCallback, getRenderTargetSizeCallback)
	{
	}

	void PongGame::Initialize()
	{
		SpriteManager::Initialize(*this);		
		
		BlendStates::Initialize(mDirect3DDevice.Get());

		mKeyboard = make_shared<KeyboardComponent>(*this);
		mComponents.push_back(mKeyboard);
		mServices.AddService(KeyboardComponent::TypeIdClass(), mKeyboard.get());

		mBall = make_shared<Ball>(*this);
		mComponents.push_back(mBall);

		mPaddle1 = make_shared<Paddle>(*this);
		mComponents.push_back(mPaddle1);

		mPaddle2 = make_shared<Paddle>(*this);
		mPaddle2->SetPlayer(2);
		mComponents.push_back(mPaddle2);

		mFont = make_shared<SpriteFont>(mDirect3DDevice.Get(), L"Content\\Fonts\\Arial_36_Regular.spritefont");
		
		srand((unsigned int)time(NULL));	

		Game::Initialize();

	}

	void PongGame::Shutdown()
	{
		BlendStates::Shutdown();
		SpriteManager::Shutdown();
	}

	void PongGame::Update(const GameTime &gameTime)
	{
		if (mKeyboard->WasKeyPressedThisFrame(Keys::Escape))
		{
			Exit();
		}

		XMFLOAT2 tempViewportSize(mViewport.Width, mViewport.Height);
		XMVECTOR viewportSize = XMLoadFloat2(&tempViewportSize);

		// Did the ball hit a paddle?
		if (mBall->Bounds().Intersects(mPaddle1->Bounds()) ||
			mBall->Bounds().Intersects(mPaddle2->Bounds()))
		{
			if (!isIntersecting)
			{
				mBall->Velocity().x *= -1;

				// this makes it so velocity only changes the one time
				isIntersecting = true; 

				// TODO Make bloop/blip/bleep
			}
		}
		else
		{
			isIntersecting = false;
		}
			

		if (!mGameOver && mBall->DidPlayerScore(Library::Players::Player1))
		{
			mPlayer1Score++;
			if (mPlayer1Score < MAXSCORE)
			{
				mBall->Initialize();				
			}
			else mGameOver = true;
		}
		else if (!mGameOver && mBall->DidPlayerScore(Library::Players::Player2))
		{
			mPlayer2Score++;
			if (mPlayer2Score < MAXSCORE) mBall->Initialize();
			else mGameOver = true;
		}

		if (!mGameOver &&
			((mBall->Velocity().y < 0 && mPaddle2->Velocity().y >= 0) ||
			(mBall->Velocity().y > 0 && mPaddle2->Velocity().y <= 0)))
		{
			// randomly choose a new velocity for y
			int32_t yModifier = rand() % 2;

			if (mPaddle2->Velocity().y == 0)
			{
				mPaddle2->ResetVelocity();
			}

			if (mBall->Bounds().Y + mBall->Bounds().Height >= mPaddle2->Bounds().Y + mPaddle2->Bounds().Y ||
				mBall->Bounds().Y <= mPaddle2->Bounds().Y)
			{
				yModifier *= 1;
			}
				
			mPaddle2->Velocity().y *= yModifier;
		}

		if (mGameOver)
		{
			mPaddle1->StopMotion();
			mPaddle2->StopMotion();

			XMVECTOR messageSize = mFont->MeasureString(mGameOverText.c_str());
			XMStoreFloat2(&mGameOverTextPosition, (viewportSize - messageSize) / 2);
			mGameOverTextPosition.y -= XMVectorGetY(messageSize);
		}

		wostringstream subMessageStream1;
		subMessageStream1 << mPlayer1Score;

		wostringstream subMessageStream2;
		subMessageStream2 << mPlayer2Score;

		// update player 1 text
		mPlayer1ScoreText = subMessageStream1.str();
		XMVECTOR messageSize = mFont->MeasureString(mPlayer1ScoreText.c_str());
		XMStoreFloat2(&mPlayer1ScoreTextPosition, (viewportSize - messageSize) / 2);
		mPlayer1ScoreTextPosition.x -= 150;
		mPlayer1ScoreTextPosition.y = 50;

		// update player 2 text
		mPlayer2ScoreText = subMessageStream2.str();
		messageSize = mFont->MeasureString(mPlayer2ScoreText.c_str());
		XMStoreFloat2(&mPlayer2ScoreTextPosition, (viewportSize - messageSize) / 2);
		mPlayer2ScoreTextPosition.x += 150;
		mPlayer2ScoreTextPosition.y = 50;

		


		Game::Update(gameTime);
	}

	void PongGame::Draw(const GameTime &gameTime)
	{
		mDirect3DDeviceContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&BackgroundColor));
		mDirect3DDeviceContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
				
		Game::Draw(gameTime);

		// draw the new scores
		SpriteManager::DrawString(mFont, mPlayer1ScoreText.c_str(), mPlayer1ScoreTextPosition);
		SpriteManager::DrawString(mFont, mPlayer2ScoreText.c_str(), mPlayer2ScoreTextPosition);

		if (mGameOver)
		{
			SpriteManager::DrawString(mFont, mGameOverText.c_str(), mGameOverTextPosition);
		}

		HRESULT hr = mSwapChain->Present(1, 0);
		
		// If the device was removed either by a disconnection or a driver upgrade, we must recreate all device resources.
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			HandleDeviceLost();
		}
		else
		{
			ThrowIfFailed(hr, "IDXGISwapChain::Present() failed.");
		}
	}

	void PongGame::Exit()
	{
		PostQuitMessage(0);
	}
}