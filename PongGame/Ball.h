#pragma once

#include "DrawableGameComponent.h"
#include "Rectangle.h"
#include <d3d11_2.h>
#include <DirectXMath.h>
#include <wrl.h>

namespace Library
{
	class KeyboardComponent;

	enum class Players
	{
		Player1 = 1,
		Player2 = 2,
	};
}

namespace Pong
{
	class Ball final : public Library::DrawableGameComponent
	{
	public:
		Ball(Library::Game& game);

		const Library::Rectangle& Bounds() const;
		DirectX::XMFLOAT2& Velocity();

		virtual void Initialize() override;
		virtual void Update(const Library::GameTime& gameTime) override;
		virtual void Draw(const Library::GameTime& gameTime) override;

		bool DidPlayerScore(Library::Players player) const;
		bool DidPlayer1Score() const;
		bool DidPlayer2Score() const;

		void Reset();

	private:
		static const int MinBallSpeed;
		static const int MaxBallSpeed;

		static std::random_device sDevice;
		static std::default_random_engine sGenerator;
		static std::uniform_int_distribution<int> sBoolDistribution;
		static std::uniform_int_distribution<int> sSpeedDistribution;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mTexture;
		Library::Point mTextureHalfSize;
		Library::Rectangle mBounds;
		DirectX::XMFLOAT2 mVelocity;
		Library::KeyboardComponent* mKeyboard;

		bool mPlayer1Scored;
		bool mPlayer2Scored;
	};
}