﻿#include <iostream>

#include "imgui-SFML.h"
#include "imgui.h"
#include <SFML/Graphics.hpp>

#include <functional>
#include <vector>
#include <thread>
#include <omp.h>

float determinant3x3(const std::vector<std::vector<float>>& matrix) {
	if (matrix.size() != 3 || matrix[0].size() != 3 || matrix[1].size() != 3 || matrix[2].size() != 3) {
		throw std::runtime_error("Matrix dimensionality is not 3x3");
	}

	return
		matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) -
		matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0]) +
		matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);;
}

sf::Color interpolateColors(const sf::Color& color1, const sf::Color& color2, float t) {
	float r = color1.r + (color2.r - color1.r) * t;
	float g = color1.g + (color2.g - color1.g) * t;
	float b = color1.b + (color2.b - color1.b) * t;
	float a = color1.a + (color2.a - color1.a) * t;

	return sf::Color(static_cast<sf::Uint8>(r), static_cast<sf::Uint8>(g), static_cast<sf::Uint8>(b), static_cast<sf::Uint8>(a));
}

class RFuncSprite : public sf::Sprite
{
public:
	void Create(const sf::Vector2u& size, const int selectedNormalIndex)
	{
		_image.create(size.x, size.y, sf::Color::Cyan);
		_texture.loadFromImage(_image);
		setTexture(_texture);

		_firstColor = sf::Color::Black;
		_secondColor = sf::Color::White;

		_selectedNormalIndex = selectedNormalIndex;
	}

	void DrawRFunc(const std::function<float(const sf::Vector2f&)>& rfunc, const sf::FloatRect& subSpace)
	{
		sf::Vector2f spaceStep = {
			subSpace.width / static_cast<float>(_image.getSize().x),
			subSpace.height / static_cast<float>(_image.getSize().y)
		};

#pragma omp parallel for
		for (int x = 0; x < _image.getSize().x - 1; ++x)
		{
			for (int y = 0; y < _image.getSize().y - 1; ++y)
			{
				sf::Vector2f spacePoint1 = {
					subSpace.left + static_cast<float>(x) * spaceStep.x,
					subSpace.top + static_cast<float>(y) * spaceStep.y
				};

				const float z1 = rfunc(spacePoint1);

				sf::Vector2f spacePoint2 = {
					subSpace.left + static_cast<float>(x + 1) * spaceStep.x,
					subSpace.top + static_cast<float>(y) * spaceStep.y
				};

				const float z2 = rfunc(spacePoint2);

				sf::Vector2f spacePoint3 = {
					subSpace.left + static_cast<float>(x) * spaceStep.x,
					subSpace.top + static_cast<float>(y + 1) * spaceStep.y
				};

				const float z3 = rfunc(spacePoint3);

				const float A = determinant3x3({
					{spacePoint1.y, z1, 1},
					{spacePoint2.y, z2, 1},
					{spacePoint3.y, z3, 1},
					});

				const float B = determinant3x3({
					{spacePoint1.x, z1, 1},
					{spacePoint2.x, z2, 1},
					{spacePoint3.x, z3, 1},
					});

				const float C = determinant3x3({
					{spacePoint1.x, spacePoint1.y, 1},
					{spacePoint2.x, spacePoint2.y, 1},
					{spacePoint3.x, spacePoint3.y, 1},
					});

				const float D = determinant3x3({
					{spacePoint1.x, spacePoint1.y, z1},
					{spacePoint2.x, spacePoint2.y, z2},
					{spacePoint3.x, spacePoint3.y, z3},
					});

				const float lenPv = std::sqrt(A * A + B * B + C * C + D * D);

				float nx = A / lenPv;
				float ny = B / lenPv;
				float nz = C / lenPv;
				float nw = D / lenPv;

				float selectedNormal = nx;

				switch (_selectedNormalIndex)
				{
				case 0:
					break;
				case 1:
					selectedNormal = ny;
					break;
				case 2:
					selectedNormal = nz;
					break;
				case 3:
					selectedNormal = nw;
					break;
				}

				auto pixelColor = interpolateColors(_firstColor, _secondColor, (1.f + selectedNormal) / 2);
				_image.setPixel(x, y, pixelColor);
			}
		}

		_texture.update(_image);
	}

	void UpdatePalette(const sf::Color& firstColor, const sf::Color& secondColor)
	{
		for (int x = 0; x < _image.getSize().x - 1; ++x)
		{
			for (int y = 0; y < _image.getSize().y - 1; ++y)
			{
				float t = (static_cast<float>(_image.getPixel(x, y).r) - _firstColor.r) / (_secondColor.r - _firstColor.r);
				auto pixelColor = interpolateColors(firstColor, secondColor, t);
				_image.setPixel(x, y, pixelColor);
			}
		}

		_firstColor = firstColor;
		_secondColor = secondColor;
		_texture.update(_image);
	}

	void SaveImageToFile(const std::string& filename)
	{
		_image.saveToFile(filename);
	}

private:
	sf::Color _firstColor;
	sf::Color _secondColor;

	sf::Texture _texture;
	sf::Image _image;

	int _selectedNormalIndex;
};

float RAnd(float w1, float w2) {
	return w1 + w2 + std::sqrt((w1 * w1 + w2 * w2) - 2 * w1 * w2);
}

float ROr(float w1, float w2) {
	return w1 + w2 - std::sqrt((w1 * w1 + w2 * w2) - 2 * w1 * w2);
}

std::vector<RFuncSprite*> sprites;

std::vector<std::thread> drawThreads;
void DrawRFuncParallel(RFuncSprite* sprite, const std::function<float(const sf::Vector2f&)>& rfunc, const sf::FloatRect& subSpace)
{
	sprite->DrawRFunc(rfunc, subSpace);
}

std::vector<std::thread> updateThreads;
void UpdatePaletteParallel(RFuncSprite* sprite, const sf::Color& firstColor, const sf::Color& secondColor)
{
	sprite->UpdatePalette(firstColor, secondColor);
}

int main()
{
	sf::RenderWindow window(sf::VideoMode(400, 400), "Lab 2");
	window.setFramerateLimit(60);
	if (!ImGui::SFML::Init(window))
	{
		std::cout << "ImGui initialization failed\n";
		return -1;
	}

	auto spriteSize = sf::Vector2u{ window.getSize().x / 2, window.getSize().y / 2 };

	RFuncSprite rFuncSpriteNx;
	rFuncSpriteNx.Create(spriteSize, 0);
	sprites.push_back(&rFuncSpriteNx);

	RFuncSprite rFuncSpriteNy;
	rFuncSpriteNy.Create(spriteSize, 1);
	rFuncSpriteNy.setPosition(spriteSize.x, 0);
	sprites.push_back(&rFuncSpriteNy);

	RFuncSprite rFuncSpriteNz;
	rFuncSpriteNz.Create(spriteSize, 2);
	rFuncSpriteNz.setPosition(0, spriteSize.y);
	sprites.push_back(&rFuncSpriteNz);

	RFuncSprite rFuncSpriteNw;
	rFuncSpriteNw.Create(spriteSize, 3);
	rFuncSpriteNw.setPosition(spriteSize.x, spriteSize.y);
	sprites.push_back(&rFuncSpriteNw);

	std::function<float(const sf::Vector2f&)> rFunctions[5];

	rFunctions[0] = [](const sf::Vector2f& point) -> float {
		return std::sin(point.x) + std::cos(point.y);
		};
	rFunctions[1] = [](const sf::Vector2f& point) -> float {
		return std::cos(point.x) * std::sin(point.y);
		};
	rFunctions[2] = [](const sf::Vector2f& point) -> float {
		return std::cos(point.x + point.y);
		};
	rFunctions[3] = [](const sf::Vector2f& point) -> float {
		return point.x * point.x + point.y * point.y - 200;
		};
	rFunctions[4] = [](const sf::Vector2f& point) -> float {
		return std::sin(point.x) * std::cos(point.y);
		};

	std::function<float(const sf::Vector2f&)> complexFunction = [&rFunctions](const sf::Vector2f& point) -> float {
		return RAnd(RAnd(ROr(RAnd(rFunctions[0](point), rFunctions[1](point)), rFunctions[2](point)), rFunctions[3](point)), rFunctions[4](point));
		};


	sf::FloatRect subSpace(-10.f, -10.f, 20.f, 20.f);

	static ImVec4 firstColor(0, 0, 0, 1);
	static ImVec4 secondColor(1, 1, 1, 1);

	for (RFuncSprite* sprite : sprites)
	{
		std::thread t(DrawRFuncParallel, sprite, complexFunction, subSpace);
		drawThreads.push_back(std::move(t));
	}

	for (std::thread& t : drawThreads)
	{
		t.join();
	}

	drawThreads.clear();

	sf::Clock deltaClock;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			ImGui::SFML::ProcessEvent(event);

			if (event.type == sf::Event::Closed)
			{
				window.close();
			}
		}

		ImGui::SFML::Update(window, deltaClock.restart());

		ImGui::Begin("Controls");

		if (ImGui::ColorEdit3("First color", &firstColor.x)) {}
		if (ImGui::ColorEdit3("Second color", &secondColor.x)) {}

		if (ImGui::Button("Update"))
		{
			auto sfFirstColor = sf::Color(
				static_cast<sf::Uint8>(firstColor.x * 255),
				static_cast<sf::Uint8>(firstColor.y * 255),
				static_cast<sf::Uint8>(firstColor.z * 255),
				static_cast<sf::Uint8>(firstColor.w * 255)
			);

			auto sfSecondColor = sf::Color(
				static_cast<sf::Uint8>(secondColor.x * 255),
				static_cast<sf::Uint8>(secondColor.y * 255),
				static_cast<sf::Uint8>(secondColor.z * 255),
				static_cast<sf::Uint8>(secondColor.w * 255)
			);

			for (RFuncSprite* sprite : sprites)
			{
				std::thread t(UpdatePaletteParallel, sprite, sfFirstColor, sfSecondColor);
				updateThreads.push_back(std::move(t));
			}

			for (std::thread& t : updateThreads)
			{
				t.join();
			}

			updateThreads.clear();
		}

		if (ImGui::Button("Save Image"))
		{
			rFuncSpriteNx.SaveImageToFile("nx.png");
			rFuncSpriteNy.SaveImageToFile("ny.png");
			rFuncSpriteNz.SaveImageToFile("nz.png");
			rFuncSpriteNw.SaveImageToFile("nw.png");
		}

		ImGui::End();

		window.clear();

		window.draw(rFuncSpriteNx);
		window.draw(rFuncSpriteNy);
		window.draw(rFuncSpriteNz);
		window.draw(rFuncSpriteNw);

		ImGui::SFML::Render(window);

		window.display();
	}

	ImGui::SFML::Shutdown();

	return 0;
}