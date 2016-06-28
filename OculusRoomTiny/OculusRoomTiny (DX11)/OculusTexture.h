#pragma once

//------------------------------------------------------------
// ovrSwapTextureSet wrapper class that also maintains the render target views
// needed for D3D11 rendering.
struct OculusTexture
{
	ovrHmd                   hmd;
	ovrSwapTextureSet      * TextureSet;
	static const int         TextureCount = 2;
	ID3D11RenderTargetView * TexRtv[TextureCount];

	OculusTexture() :
		hmd(nullptr),
		TextureSet(nullptr)
	{
		TexRtv[0] = TexRtv[1] = nullptr;
	}

	bool Init(ovrHmd _hmd, int sizeW, int sizeH)
	{
		hmd = _hmd;

		D3D11_TEXTURE2D_DESC dsDesc;
		dsDesc.Width = sizeW;
		dsDesc.Height = sizeH;
		dsDesc.MipLevels = 1;
		dsDesc.ArraySize = 1;
		dsDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		dsDesc.SampleDesc.Count = 1;   // No multi-sampling allowed
		dsDesc.SampleDesc.Quality = 0;
		dsDesc.Usage = D3D11_USAGE_DEFAULT;
		dsDesc.CPUAccessFlags = 0;
		dsDesc.MiscFlags = 0;
		dsDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

		ovrResult result =
			ovr_CreateSwapTextureSetD3D11(hmd, DIRECTX.Device, &dsDesc,
				ovrSwapTextureSetD3D11_Typeless,
				&TextureSet);
		if (!OVR_SUCCESS(result))
			return false;

		VALIDATE(TextureSet->TextureCount == TextureCount,
			"TextureCount mismatch.");

		for (int i = 0; i < TextureCount; ++i)
		{
			ovrD3D11Texture* tex = (ovrD3D11Texture*)&TextureSet->Textures[i];
			D3D11_RENDER_TARGET_VIEW_DESC rtvd = {};
			rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			DIRECTX.Device->CreateRenderTargetView(tex->D3D11.pTexture, &rtvd,
				&TexRtv[i]);
		}

		return true;
	}

	~OculusTexture()
	{
		for (int i = 0; i < TextureCount; ++i)
		{
			Release(TexRtv[i]);
		}
		if (TextureSet)
		{
			ovr_DestroySwapTextureSet(hmd, TextureSet);
		}
	}

	void AdvanceToNextTexture()
	{
		TextureSet->CurrentIndex =
			(TextureSet->CurrentIndex + 1) % TextureSet->TextureCount;
	}
};