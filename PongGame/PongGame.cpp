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
		mBlip[0] = std::make_unique<SoundEffect>(mAudio->AudioEngine().get(), L"Content\\Audio\\PongBlip1.wav");
		mBlip[1] = std::make_unique<SoundEffect>(mAudio->AudioEngine().get(), L"Content\\Audio\\PongBlip2.wav");
		mBlip[2] = std::make_unique<SoundEffect>(mAudio->AudioEngine().get(), L"Content\\Audio\\PongBlip3.wav");
		mBlip[3] = std::make_unique<SoundEffect>(mAudio->AudioEngine().get(), L"Content\\Audio\\PongBlip4.wav");
		mBlip[4] = std::make_unique<SoundEffect>(mAudio->AudioEngine().get(), L"Content\\Audio\\PongBlip5.wav");
		mBlip[5] = std::make_unique<SoundEffect>(mAudio->AudioEngine().get(), L"Content\\Audio\\PongBlip6.wav");
		mGameOverSound = std::make_unique<SoundEffect>(mAudio->AudioEngine().get(), L"Content\\Audio\\PongGameOver.wav");
		mScoreSound = std::make_unique<SoundEffect>(mAudio->AudioEngine().get(), L"Content\\Audio\\PongScore.wav");
		mFont = make_shared<SpriteFont>(mDirect3DDevice.Get(), L"Content\\Fonts\\Arial_36_Regular.spritefont");
		mSmallFont = make_shared<SpriteFont>(mDirect3DDevice.Get(), L"Content\\Fonts\\Arial_14_Regular.spritefont");
		
		srand((unsigned int)time(NULL));	

		Game::Initialize();
		FreezeMotion();
	}

	void PongGame::Shutdown()
	{
		BlendStates::Shutdown();
		SpriteManager::Shutdown();
	}

	void PongGame::Update(const GameTime &gameTime)
	{
		HandleKeyboardInput();

		if (mGamestate == Gamestate::Initial)
		{
			ShowDirectionsText();
			ShowLogoText();
		}
		if (mGamestate == Gamestate::Playing)
		{
			HandleBallPhysics();
			AdjustAIPaddleVelocity(gameTime);
			UpdatePlayerScores();
		}		
		if (mGamestate == Gamestate::Gameover)
		{
			ShowGameOver();
			ShowDirectionsText();
		}

		Game::Update(gameTime);
	}

	void PongGame::Draw(const GameTime &gameTime)
	{
		mDirect3DDeviceContext->ClearRenderTargetView(mRenderTargetView.Get(), reinterpret_cast<const float*>(&BackgroundColor));
		mDirect3DDeviceContext->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		Game::Draw(gameTime);

		if (mGamestate == Gamestate::Initial)
		{
			SpriteManager::DrawString(mFont, mPongText.c_str(), mPongTextPosition);
			SpriteManager::DrawString(mSmallFont, mDirectionsText.c_str(), mDirectionsTextPosition);
		}
		else if (mGamestate == Gamestate::Playing)
		{
			SpriteManager::DrawString(mFont, mPlayer1ScoreText.c_str(), mPlayer1ScoreTextPosition);
			SpriteManager::DrawString(mFont, mPlayer2ScoreText.c_str(), mPlayer2ScoreTextPosition);
		}
		else if (mGamestate == Gamestate::Gameover)
		{
			SpriteManager::DrawString(mFont, mGameOverText.c_str(), mGameOverTextPosition);
			SpriteManager::DrawString(mSmallFont, mDirectionsText.c_str(), mDirectionsTextPosition);
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

	void PongGame::ChangeGameState(Gamestate newGamestate)
	{
		if (mGamestate == Gamestate::Initial || mGamestate == Gamestate::Gameover)
		{
			// transitioning to playing
			Game::Initialize();
			mPlayer1Score = 0;
			mPlayer2Score = 0;
		}
		else if (mGamestate == Gamestate::Playing)
		{
			// transitioning to gameover
			FreezeMotion();
			MakeGameOverSound();
		}

		mGamestate = newGamestate;
	}

	bool PongGame::IsBallAboveAIPaddle()
	{
		return mBall->Bounds().Bottom() < mPaddle2->Bounds().Top();
	}

	bool PongGame::IsBallBelowAIPaddle()
	{
		return mBall->Bounds().Top() > mPaddle2->Bounds().Bottom();
	}

	void PongGame::HandleKeyboardInput()
	{
		if (mKeyboard->WasKeyPressedThisFrame(Keys::Escape))
		{
			Exit();
		}

		if (mGamestate != Gamestate::Playing)
		{
			if (mKeyboard->WasKeyPressedThisFrame(Keys::Space))
			{
				ChangeGameState(Gamestate::Playing);
			}
		}
	}

	void PongGame::HandleBallPhysics()
	{
		bool paddle1Intersects = mBall->Bounds().Intersects(mPaddle1->Bounds());
		bool paddle2Intersects = mBall->Bounds().Intersects(mPaddle2->Bounds());

		// Did the ball hit a paddle?		
		if (paddle1Intersects || paddle2Intersects)
		{
			if (paddle2Intersects)
			{
				mPaddle2->Velocity().y = 0.0f;
			}

			if (!mIsIntersecting)
			{
				mBall->Velocity().x *= -1.0f;

				// this makes it so velocity only changes the one time
				mIsIntersecting = true;

				MakeBlip();
			}
		}
		else
		{
			mIsIntersecting = false;
		}

		// Did the ball hit a wall?
		if (mBall->DidBallHitWall()) MakeBlip();
	}

	void PongGame::AdjustAIPaddleVelocity(const GameTime &gameTime)
	{
		// don't attempt to follow if the ball is going the other way
		if (mBall->Velocity().x < 0 || int(gameTime.TotalGameTimeSeconds().count()) % mAIDelay == 0)
		{
			mPaddle2->Velocity().y = 0.0f;
		}
		else if (IsBallBelowAIPaddle() && mPaddle2->Velocity().y <= 0)
		{
			mPaddle2->ResetVelocity();			
		}
		else if (IsBallAboveAIPaddle() && mPaddle2->Velocity().y >= 0)
		{
			mPaddle2->ResetVelocity();
			mPaddle2->Velocity().y *= -1;
		}		
	}

	void PongGame::ShowGameOver()
	{
		XMFLOAT2 tempViewportSize(mViewport.Width, mViewport.Height);
		XMVECTOR viewportSize = XMLoadFloat2(&tempViewportSize);

		// display the game over text
		XMVECTOR messageSize = mFont->MeasureString(mGameOverText.c_str());
		XMStoreFloat2(&mGameOverTextPosition, (viewportSize - messageSize) / 2);
		mGameOverTextPosition.y -= XMVectorGetY(messageSize);		
	}

	void PongGame::ShowDirectionsText()
	{
		XMFLOAT2 tempViewportSize(mViewport.Width, mViewport.Height);
		XMVECTOR viewportSize = XMLoadFloat2(&tempViewportSize);

		// display the directions text
		XMVECTOR messageSize = mSmallFont->MeasureString(mDirectionsText.c_str());
		XMVECTOR gameOverMessageSize = mFont->MeasureString(mGameOverText.c_str());
		XMStoreFloat2(&mDirectionsTextPosition, (viewportSize - messageSize) / 2);
		mDirectionsTextPosition.y -= (XMVectorGetY(messageSize) - (XMVectorGetY(gameOverMessageSize) * 1.05f));
	}

	void PongGame::ShowLogoText()
	{
		XMFLOAT2 tempViewportSize(mViewport.Width, mViewport.Height);
		XMVECTOR viewportSize = XMLoadFloat2(&tempViewportSize);

		// display the game over text
		XMVECTOR messageSize = mFont->MeasureString(mPongText.c_str());
		XMStoreFloat2(&mPongTextPosition, (viewportSize - messageSize) / 2);
		mPongTextPosition.y -= XMVectorGetY(messageSize);
	}
	
	void PongGame::FreezeMotion()
	{
		mPaddle1->StopMotion();
		mPaddle2->StopMotion();
		mBall->StopMotion();
	}

	void PongGame::UpdatePlayerScores()
	{
		// did a player score?
		if (mBall->DidPlayerScore(Library::Players::Player1))
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
				ChangeGameState(Gamestate::Gameover);				
			}
		}
		else if (mBall->DidPlayerScore(Library::Players::Player2))
		{
			mPlayer2Score++;
			if (mPlayer2Score < MAXSCORE)
			{
				MakeScoreSound();
				mBall->Initialize();
			}
			else
			{
				ChangeGameState(Gamestate::Gameover);
			}
		}

		XMFLOAT2 tempViewportSize(mViewport.Width, mViewport.Height);
		XMVECTOR viewportSize = XMLoadFloat2(&tempViewportSize);

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

}