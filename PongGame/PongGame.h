#pragma once

#include "Game.h"
#include "Rectangle.h"

namespace Library
{
	class KeyboardComponent;
}

namespace Pong
{
	enum class Gamestate
	{
		Initial = 1,
		Playing = 2,
		Gameover = 3,
	};

	class Ball;
	class Paddle;

	class PongGame : public Library::Game
	{
	public:
		PongGame(std::function<void*()> getWindowCallback, std::function<void(SIZE&)> getRenderTargetSizeCallback);

		virtual void Initialize() override;
		virtual void Shutdown() override;
		virtual void Update(const Library::GameTime& gameTime) override;		
		virtual void Draw(const Library::GameTime& gameTime) override;

	private:
		void Exit();
		void MakeBlip();
		void MakeGameOverSound();
		void MakeScoreSound();
		void UpdatePlayerScores();
		void ShowGameOver();
		void AdjustAIPaddleVelocity();
		void HandleBallPhysics();
		void HandleKeyboardInput();
		void FreezeMotion();
		void ShowDirectionsText();
		void ShowLogoText();
		void ChangeGameState(Gamestate newGamestate);
		bool IsBallAboveAIPaddle();
		bool IsBallBelowAIPaddle();

		static const DirectX::XMVECTORF32 BackgroundColor;

		std::shared_ptr<Library::AudioEngineComponent> mAudio;
		std::unique_ptr<DirectX::SoundEffect> mBlip[6];
		std::unique_ptr<DirectX::SoundEffect> mScoreSound;
		std::unique_ptr<DirectX::SoundEffect> mGameOverSound;
		std::shared_ptr<Library::KeyboardComponent> mKeyboard;
		std::shared_ptr<Ball> mBall;
		std::shared_ptr<Paddle> mPaddle1;
		std::shared_ptr<Paddle> mPaddle2;
		std::shared_ptr<DirectX::SpriteFont> mFont;
		std::shared_ptr<DirectX::SpriteFont> mSmallFont;
		std::wstring mPlayer1ScoreText;
		std::wstring mPlayer2ScoreText;
	    const std::wstring mGameOverText = L"Game Over!";
		const std::wstring mPongText = L"PONG";
		const std::wstring mDirectionsText = L"Press SPACEBAR to play";
		DirectX::XMFLOAT2 mPlayer1ScoreTextPosition;
		DirectX::XMFLOAT2 mPlayer2ScoreTextPosition;
		DirectX::XMFLOAT2 mGameOverTextPosition;		
		DirectX::XMFLOAT2 mPongTextPosition;
		DirectX::XMFLOAT2 mDirectionsTextPosition;

		int32_t mPlayer1Score = 0;
		int32_t mPlayer2Score = 0;
		float lastAIChange = -5.0f;
		bool isIntersecting = false;

		Gamestate mGamestate = Gamestate::Initial;
	};
}