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
	const XMVECTORF32 PongGame::BackgroundColor = Colors::SteelBlue;
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

		mAudio = make_shared<AudioEngineComponent>(*this);
		mComponents.push_back(mAudio);
		mServices.AddService(AudioEngineComponent::TypeIdClass(), mAudio.get());
				
		mBall = make_shared<Ball>(*this);
		mComponents.push_back(mBall);

		mPaddle1 = make_shared<Paddle>(*this);
		mComponents.push_back(mPaddle1);

		mPaddle2 = make_shared<Paddle>(*this);
		mPaddle2->SetPlayer(2);
		mComponents.push_back(mPaddle2);

		// Add the sound effects and font
		mBlip[0] = std::make_shared<SoundEffect>(mAudio->AudioEngine().get(), L"Content\\Audio\\PongBlip1.wav");
		mBlip[1] = std::make_shared<SoundEffect>(mAudio->AudioEngine().get(), L"Content\\Audio\\PongBlip2.wav");
		mBlip[2] = std::make_shared<SoundEffect>(mAudio->AudioEngine().get(), L"Content\\Audio\\PongBlip3.wav");
		mBlip[3] = std::make_shared<SoundEffect>(mAudio->AudioEngine().get(), L"Content\\Audio\\PongBlip4.wav");
		mBlip[4] = std::make_shared<SoundEffect>(mAudio->AudioEngine().get(), L"Content\\Audio\\PongBlip5.wav");
		mBlip[5] = std::make_shared<SoundEffect>(mAudio->AudioEngine().get(), L"Content\\Audio\\PongBlip6.wav");
		mGameOverSound = std::make_shared<SoundEffect>(mAudio->AudioEngine().get(), L"Content\\Audio\\PongGameOver.wav");
		mScoreSound = std::make_shared<SoundEffect>(mAudio->AudioEngine().get(), L"Content\\Audio\\PongScore.wav");
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

		// Did the ball hit a paddle?
		if (mBall->Bounds().Intersects(mPaddle1->Bounds()) ||
			mBall->Bounds().Intersects(mPaddle2->Bounds()))
		{
			if (!isIntersecting)
			{
				mBall->Velocity().x *= -1;

				// this makes it so velocity only changes the one time
				isIntersecting = true; 

				MakeBlip();				
			}
		}
		else
		{
			isIntersecting = false;
		}

		// Did the ball hit a wall?
		if (mBall->DidBallHitWall()) MakeBlip();
			
		// did a player score?
		if (!mGameOver && mBall->DidPlayerScore(Library::Players::Player1))
		{
			MakeScoreSound();
			mPlayer1Score++;
			if (mPlayer1Score < MAXSCORE)
			{
				MakeScoreSound();
				mBall->Initialize();
			}
			else
			{
				MakeGameOverSound();
				mGameOver = true;
			}
		}
		else if (!mGameOver && mBall->DidPlayerScore(Library::Players::Player2))
		{			
			mPlayer2Score++;
			if (mPlayer2Score < MAXSCORE)
			{
				MakeScoreSound();
				mBall->Initialize();
			}
			else
			{
				MakeGameOverSound();
				mGameOver = true;
			}
		}

		// this randomly adjusts velocity of AI paddle
		bool ballChangedDirection = ((mBall->Velocity().y < 0 && mPaddle2->Velocity().y >= 0) ||
			(mBall->Velocity().y > 0 && mPaddle2->Velocity().y <= 0));
		int32_t yModifier = rand() % 2;

		if (!mGameOver && ballChangedDirection)
		{
			if (yModifier == 0) mPaddle2->ResetVelocity();
		}

		if ((mBall->Velocity().y < 0 && mPaddle2->Velocity().y > 0) ||
			(mBall->Velocity().y > 0 && mPaddle2->Velocity().y < 0))
		{
			if (yModifier == 0) mPaddle2->Velocity().y *= -1;
		}


		XMFLOAT2 tempViewportSize(mViewport.Width, mViewport.Height);
		XMVECTOR viewportSize = XMLoadFloat2(&tempViewportSize);

		// did the game end?
		if (mGameOver)
		{
			// freeze motion
			mPaddle1->StopMotion();
			mPaddle2->StopMotion();
			mBall->StopMotion();

			// display the game over text
			XMVECTOR messageSize = mFont->MeasureString(mGameOverText.c_str());
			XMStoreFloat2(&mGameOverTextPosition, (viewportSize - messageSize) / 2);
			mGameOverTextPosition.y -= XMVectorGetY(messageSize);
		}

		// update the score texts
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

	void PongGame::MakeBlip()
	{
		int32_t chooseBlip = rand() % 5;
		mBlip[chooseBlip]->Play();
	}

	void PongGame::MakeGameOverSound()
	{
		mGameOverSound->Play();
	}

	void PongGame::MakeScoreSound()
	{
		mScoreSound->Play();
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