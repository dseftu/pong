#include "pch.h"
#include "Ball.h"

using namespace DirectX;
using namespace Library;
using namespace std;
using namespace Microsoft::WRL;

namespace Pong
{
	const int Ball::MinBallSpeed = 200;
	const int Ball::MaxBallSpeed = 400;

	random_device Ball::sDevice;
	default_random_engine Ball::sGenerator(sDevice());
	uniform_int_distribution<int> Ball::sBoolDistribution(0, 1);
	uniform_int_distribution<int> Ball::sSpeedDistribution(MinBallSpeed, MaxBallSpeed);

	Ball::Ball(Game& game) :
		DrawableGameComponent(game), mBounds(Rectangle::Empty)
	{
	}

	const Library::Rectangle& Ball::Bounds() const
	{
		return mBounds;
	}

	DirectX::XMFLOAT2& Ball::Velocity()
	{
		return mVelocity;
	}

	void Ball::Initialize()
	{
		// Load a texture
		ComPtr<ID3D11Resource> textureResource;
		wstring textureName = L"Content\\Textures\\Ball.png";

		ThrowIfFailed(CreateWICTextureFromFile(mGame->Direct3DDevice(), textureName.c_str(), textureResource.ReleaseAndGetAddressOf(), mTexture.ReleaseAndGetAddressOf()), "CreateWICTextureFromFile() failed.");

		ComPtr<ID3D11Texture2D> texture;
		ThrowIfFailed(textureResource.As(&texture), "Invalid ID3D11Resource returned from CreateWICTextureFromFile. Should be a ID3D11Texture2D.");

		mBounds = TextureHelper::GetTextureBounds(texture.Get());
		mTextureHalfSize.X = mBounds.Width / 2;
		mTextureHalfSize.Y = mBounds.Height / 2;

		mKeyboard = reinterpret_cast<KeyboardComponent*>(mGame->Services().GetService(KeyboardComponent::TypeIdClass()));

		mPlayer1Scored = false;
		mPlayer2Scored = false;

		Reset();
	}

	void Ball::Update(const Library::GameTime& gameTime)
	{
		float elapsedTime = gameTime.ElapsedGameTimeSeconds().count();

		XMFLOAT2 positionDelta(mVelocity.x * elapsedTime, mVelocity.y * elapsedTime);
		mBounds.X += static_cast<int>(std::round(positionDelta.x));
		mBounds.Y += static_cast<int>(std::round(positionDelta.y));

		auto& viewport = mGame->Viewport();
		if (mBounds.X + mBounds.Width >= viewport.Width && mVelocity.x > 0.0f)
		{
			// TODO remove this, just keeping it for reference
			//mVelocity.x *= -1;
			mPlayer1Scored = true;
			
		}
		if (mBounds.X <= 0 && mVelocity.x < 0.0f)
		{
			mPlayer2Scored = true;
		}

		if (mBounds.Y + mBounds.Height >= viewport.Height && mVelocity.y > 0.0f)
		{
			mVelocity.y *= -1;
		}
		if (mBounds.Y <= 0 && mVelocity.y < 0.0f)
		{
			mVelocity.y *= -1;
		}
	}

	void Ball::Draw(const Library::GameTime& gameTime)
	{
		UNREFERENCED_PARAMETER(gameTime);

		XMFLOAT2 position(static_cast<float>(mBounds.X), static_cast<float>(mBounds.Y));
		SpriteManager::DrawTexture2D(mTexture.Get(), position);
	}

	bool Ball::DidPlayerScore(Library::Players player) const
	{
		if (player == Library::Players::Player1)
		{
			return DidPlayer1Score();
		}
		else
		{
			return DidPlayer2Score();
		}

	}

	bool Ball::DidPlayer1Score() const
	{
		return mPlayer1Scored;
	}

	bool Ball::DidPlayer2Score() const
	{
		return mPlayer2Scored;
	}

	void Ball::Reset()
	{
		Library::Rectangle viewportSize(static_cast<int>(mGame->Viewport().TopLeftX), static_cast<int>(mGame->Viewport().TopLeftY), static_cast<int>(mGame->Viewport().Width), static_cast<int>(mGame->Viewport().Height));
		Point center = viewportSize.Center();
		mBounds.X = center.X - mTextureHalfSize.X;
		mBounds.Y = center.Y - mTextureHalfSize.Y;

		mVelocity.x = static_cast<float>(sSpeedDistribution(sGenerator) * (sBoolDistribution(sGenerator) ? 1 : -1));
		mVelocity.y = static_cast<float>(sSpeedDistribution(sGenerator) * (sBoolDistribution(sGenerator) ? 1 : -1));
	}
}