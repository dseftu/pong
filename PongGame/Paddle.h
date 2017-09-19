#pragma once

#include "DrawableGameComponent.h"
#include "Rectangle.h"
#include <d3d11_2.h>
#include <DirectXMath.h>
#include <wrl.h>

namespace Library
{
	class KeyboardComponent;
}

namespace Pong
{
	class Paddle final : public Library::DrawableGameComponent
	{
	public:
		Paddle(Library::Game& game);

		const Library::Rectangle& Bounds() const;
		DirectX::XMFLOAT2& Velocity();

		virtual void Initialize() override;
		virtual void SetPlayer(int mPlayer);
		virtual void Update(const Library::GameTime& gameTime) override;
		void AIControl(float elapsedTime);
		void HumanControl(float elapsedTime);
		virtual void Draw(const Library::GameTime& gameTime) override;

		void Reset();
		void ResetVelocity();
		void StopMotion();

	private:
		static const float PaddleSpeed;
		static const int WallOffset;

		static std::random_device sDevice;
		static std::default_random_engine sGenerator;
		static std::uniform_int_distribution<int> sBoolDistribution;
		static std::uniform_int_distribution<int> sSpeedDistribution;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mTexture;
		Library::Point mTextureHalfSize;
		Library::Rectangle mBounds;
		DirectX::XMFLOAT2 mVelocity;
		Library::KeyboardComponent* mKeyboard;
		
		int mPlayer = 1;

	};
}