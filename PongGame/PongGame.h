#pragma once

#include "Game.h"
#include "Rectangle.h"

namespace Library
{
	class KeyboardComponent;
}

namespace Pong
{
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

		static const DirectX::XMVECTORF32 BackgroundColor;

		std::shared_ptr<Library::KeyboardComponent> mKeyboard;
		std::shared_ptr<Ball> mBall;
		std::shared_ptr<Paddle> mPaddle1;
		std::shared_ptr<Paddle> mPaddle2;
		std::shared_ptr<DirectX::SpriteFont> mFont;
		std::wstring mPlayer1ScoreText;
		std::wstring mPlayer2ScoreText;
		DirectX::XMFLOAT2 mPlayer1ScoreTextPosition;
		DirectX::XMFLOAT2 mPlayer2ScoreTextPosition;

		int mPlayer1Score = 0;
		int mPlayer2Score = 0;
		bool mGameOver = false;
		bool isIntersecting = false;
	};
}