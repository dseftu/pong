#include "pch.h"
#include "Paddle.h"

using namespace DirectX;
using namespace Library;
using namespace std;
using namespace Microsoft::WRL;

namespace Pong
{
	const int Paddle::MinPaddleSpeed = 200;
	const int Paddle::MaxPaddleSpeed = 400;
	const int Paddle::HumanControlledSpeed = 450;
	const int Paddle::WallOffset = 100;

	random_device Paddle::sDevice;
	default_random_engine Paddle::sGenerator(sDevice());
	uniform_int_distribution<int> Paddle::sBoolDistribution(0, 1);
	uniform_int_distribution<int> Paddle::sSpeedDistribution(MinPaddleSpeed, MaxPaddleSpeed);

	Paddle::Paddle(Game& game) :
		DrawableGameComponent(game), mBounds(Rectangle::Empty)
	{
	}

	const Library::Rectangle& Paddle::Bounds() const
	{
		return mBounds;
	}

	DirectX::XMFLOAT2& Paddle::Velocity()
	{
		return mVelocity;
	}

	void Paddle::Initialize()
	{
		// Load a texture
		ComPtr<ID3D11Resource> textureResource;
		wstring textureName = L"Content\\Textures\\Paddle.png";

		ThrowIfFailed(CreateWICTextureFromFile(mGame->Direct3DDevice(), textureName.c_str(), textureResource.ReleaseAndGetAddressOf(), mTexture.ReleaseAndGetAddressOf()), "CreateWICTextureFromFile() failed.");

		ComPtr<ID3D11Texture2D> texture;
		ThrowIfFailed(textureResource.As(&texture), "Invalid ID3D11Resource returned from CreateWICTextureFromFile. Should be a ID3D11Texture2D.");

		mBounds = TextureHelper::GetTextureBounds(texture.Get());
		mTextureHalfSize.X = mBounds.Width - 50;
		mTextureHalfSize.Y = mBounds.Height / 2;

		mKeyboard = reinterpret_cast<KeyboardComponent*>(mGame->Services().GetService(KeyboardComponent::TypeIdClass()));
		
		Reset();
	}

	void Paddle::SetPlayer(int player)
	{
		mPlayer = player;
	}

	void Paddle::Update(const Library::GameTime& gameTime)
	{
		float elapsedTime = gameTime.ElapsedGameTimeSeconds().count();

		if (mPlayer == 2) AIControl(elapsedTime);
		else HumanControl(elapsedTime);
	}

	void Paddle::AIControl(float elapsedTime)
	{
		auto& viewport = mGame->Viewport();				

		XMFLOAT2 positionDelta(0, mVelocity.y * elapsedTime);
		mBounds.Y += static_cast<int>(std::round(positionDelta.y));

		if (mBounds.Y + mBounds.Height >= viewport.Height && mVelocity.y > 0.0f)
		{
			mBounds.Y = (int32_t)viewport.Height - mBounds.Height;			
		}
		if (mBounds.Y <= 0 && mVelocity.y < 0.0f)
		{
			mBounds.Y = 0;
		}
	}

	void Paddle::HumanControl(float elapsedTime)
	{
		// determine if the paddle is at the edge.
		auto& viewport = mGame->Viewport();
		bool atBottomBoundary = (mBounds.Y + mBounds.Height >= viewport.Height);
		bool atTopBoundary = (mBounds.Y <= 0);

		if (mKeyboard->IsKeyDown(Keys::Up) && !atTopBoundary)
		{
			XMFLOAT2 positionDelta(0, mVelocity.y * elapsedTime);
			mBounds.Y -= static_cast<int>(std::round(positionDelta.y));
		}
		if (mKeyboard->IsKeyDown(Keys::Down) && !atBottomBoundary)
		{
			XMFLOAT2 positionDelta(0, mVelocity.y * elapsedTime);
			mBounds.Y += static_cast<int>(std::round(positionDelta.y));
		}	
	}

	void Paddle::Draw(const Library::GameTime& gameTime)
	{
		UNREFERENCED_PARAMETER(gameTime);

		XMFLOAT2 position(static_cast<float>(mBounds.X), static_cast<float>(mBounds.Y));
		SpriteManager::DrawTexture2D(mTexture.Get(), position);
	}

	void Paddle::Reset()
	{
		Library::Rectangle viewportSize(static_cast<int>(mGame->Viewport().TopLeftX), static_cast<int>(mGame->Viewport().TopLeftY), static_cast<int>(mGame->Viewport().Width), static_cast<int>(mGame->Viewport().Height));
		Point center = viewportSize.Center();
		
		mVelocity.x = 0;

		if (mPlayer == 1)
		{
			mBounds.X = WallOffset;
			mVelocity.y = HumanControlledSpeed;
		}
		else
		{
			mBounds.X = viewportSize.Right() - WallOffset;
			mVelocity.y = static_cast<float>(sSpeedDistribution(sGenerator));
		}

		mBounds.Y = center.Y - mTextureHalfSize.Y;
		
	}

	void Paddle::ResetVelocity()
	{
		mVelocity.y = static_cast<float>(sSpeedDistribution(sGenerator) * (sBoolDistribution(sGenerator) ? 1 : -1));
	}

	void Paddle::StopMotion()
	{
		mVelocity.y = 0;
		mVelocity.x = 0;
	}
}