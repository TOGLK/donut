// Copyright 2019-2020 the donut authors. See AUTHORS.md

#include "Pure3D/Texture.h"
#include <P3D/P3D.generated.h>
#include <Render/Font.h>
#include <Render/Mesh.h>
#include <Render/Shader.h>
#include <ResourceManager.h>

#include "Render/OpenGL/glad/gl.h"
#include "Render/imgui/imgui.h"
#include "ThirdParty/magic_enum.h"

#include <chrono>
#include <ctime>
#include <fmt/format.h>

namespace Donut
{

ResourceManager::ResourceManager()
{
	createErrorTexture();
}

ResourceManager::~ResourceManager()
{
	delete _errorTexture;

	for (auto texture : _textures) {
		delete texture.second;
	}
}

void ResourceManager::AddTexture(Texture* texture)
{
	_textures[texture->GetName()] = texture;
}

void ResourceManager::LoadTexture(const P3D::Sprite& sprite)
{
	// if (_textures.find(sprite.GetName()) != _textures.end())
	// 	fmt::print("Sprite {0} already loaded\n", sprite.GetName());

	_textures[sprite.GetName()] = new Texture(sprite);
}

void ResourceManager::LoadShader(const P3D::Shader& shader)
{
	// if (_shaders.find(shader.GetName()) != _shaders.end())
	// 	fmt::print("Shader {0} already loaded\n", shader.GetName());

	_shaders[shader.GetName()] = std::make_unique<Shader>(shader);
}

void ResourceManager::LoadSet(const P3D::Set& set)
{
	// if (_textures.find(set.GetName()) != _textures.end())
	// 	fmt::print("Set {0} already loaded\n", set.GetName());

	std::srand((uint32_t)std::time(0));
	int idx = std::rand() % set.GetTextures().size();
	_textures[set.GetName()] = new Texture(*set.GetTextures().at(idx));
}

void ResourceManager::LoadGeometry(const P3D::Geometry& geo)
{
	// if (_geometries.find(geo.GetName()) != _geometries.end())
	// 	fmt::print("Geometry {0} already loaded\n", geo.GetName());

	_geometries[geo.GetName()] = std::make_unique<Mesh>(geo);
}

/*void ResourceManager::AddTexture(const std::string& name, std::unique_ptr<Texture> texture)
{
    _textures[name] = std::move(texture);
}*/

void ResourceManager::AddFont(const std::string& name, std::unique_ptr<Font> font)
{
	_fonts[name] = std::move(font);
}

void ResourceManager::createErrorTexture()
{
	// nice hot pink error texture
	constexpr GLuint errorTextureData[] = {0xFFFF00DC, 0xFF000000, 0xFF000000, 0xFFFF00DC};

	_errorTexture = new Texture();
	_errorTexture->Create(2, 2, Texture::Format::RGBA8,
	                      std::vector<uint8_t>(std::begin(errorTextureData), std::end(errorTextureData)));
}

void ResourceManager::ImGuiDebugWindow(bool* p_open) const
{
	ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_Once);
	if (!ImGui::Begin("Resource Manager (P3D)", p_open))
	{
		ImGui::End();
		return;
	}

	const float footer_height_to_reserve =
	    ImGui::GetStyle().ItemSpacing.y + ImGui::GetTextLineHeightWithSpacing(); // 1 separator, 1 text line
	ImGui::BeginChild("##spacerfix", ImVec2(0, -footer_height_to_reserve), false);

	static ImGuiTextFilter filter;
	filter.Draw();

	ImGui::BeginTabBar("##rmtabs");

	if (ImGui::BeginTabItem("Textures"))
	{
		const float curWidth = ImGui::GetColumnWidth();

		ImGui::Columns(4, "##texturescol");
		ImGui::SetColumnWidth(0, curWidth - 96 - 96 - 96);
		ImGui::SetColumnWidth(1, 96);
		ImGui::SetColumnWidth(2, 96);
		ImGui::SetColumnWidth(3, 96);

		ImGui::Separator();
		ImGui::Text("Name");
		ImGui::NextColumn();
		ImGui::Text("Refs");
		ImGui::NextColumn();
		ImGui::Text("Weight");
		ImGui::NextColumn();
		ImGui::Text("Size");
		ImGui::NextColumn();
		ImGui::Separator();
		ImGui::Columns(1);

		ImGui::BeginChild("##textures");
		ImGui::Columns(4, "##texturescol");
		ImGui::SetColumnWidth(0, curWidth - 96 - 96 - 96);
		ImGui::SetColumnWidth(1, 96);
		ImGui::SetColumnWidth(2, 96);
		ImGui::SetColumnWidth(3, 96);

		int i = 0;
		for (std::pair<std::string, Texture*> pair : _textures)
		{
			auto name = pair.first;
			Texture* texture = pair.second;

			if (!filter.PassFilter(name.c_str()))
				continue;

			ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
			bool hovered = ImGui::IsItemHovered();
			if (hovered)
			{
				ImGui::BeginTooltip();
				ImGui::Image((ImTextureID)(intptr_t)texture->GetNativeTextureHandle(),
				             ImVec2((float)texture->GetWidth(), (float)texture->GetHeight()));
				ImGui::Text("Format: %s", std::string(magic_enum::enum_name(texture->GetFormat())).c_str());
				ImGui::EndTooltip();
			}

			ImGui::NextColumn();
			ImGui::Text("1");
			ImGui::NextColumn();
			ImGui::Text("%.1fKB", (float)texture->GetMemorySize() / 1024.0f);
			ImGui::NextColumn();
			ImGui::Text("%zux%zu", texture->GetWidth(), texture->GetHeight());
			ImGui::NextColumn();

			i++;
		}

		ImGui::EndChild();

		ImGui::EndTabItem();
	}

	ImGui::EndTabBar();

	ImGui::EndChild();

	ImGui::Separator();

	ImGui::TextDisabled("Textures: %zu", _textures.size());
	ImGui::SameLine();
	ImGui::TextDisabled("Shaders: %zu", _shaders.size());
	ImGui::SameLine();
	ImGui::TextDisabled("Fonts: %zu", _fonts.size());

	ImGui::End();
}

/*
 * Searches for the shader in the map, you should cache the result to avoid unnecessary lookups
 */
Shader* ResourceManager::GetShader(const std::string& name) const
{
	if (_shaders.find(name) == _shaders.end())
	{
		fmt::print("could not find shader {0}\n", name);
		return nullptr; // todo: return an error shader
	}

	auto const& shader = _shaders.at(name);

	// todo: check if a texture is set/valid before setting texture again
	const std::string texName = shader->GetDiffuseTextureName();
	if (_textures.find(texName) != _textures.end())
		shader->SetDiffuseTexture(_textures.at(texName));
	else
	{
		// fmt::print("could not find texture {1} for shader {0}\n", name, texName);
		shader->SetDiffuseTexture(_errorTexture);
	}

	return shader.get();
}

Texture* ResourceManager::GetTexture(const std::string& name) const
{
	if (_textures.find(name) == _textures.end())
		return _errorTexture;

	return _textures.at(name);
}

Mesh* ResourceManager::GetGeometry(const std::string& name) const
{
	if (_geometries.find(name) == _geometries.end())
		return nullptr;

	return _geometries.at(name).get();
}

Font* ResourceManager::GetFont(const std::string& name) const
{
	if (_fonts.find(name) == _fonts.end())
		return nullptr;

	return _fonts.at(name).get();
}

} // namespace Donut
