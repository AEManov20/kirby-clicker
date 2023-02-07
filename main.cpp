#include <entt/entt.hpp>
#include <raylib.h>
#include <raymath.h>
#include <iostream>
#include <random>

// basic wrapper around the Load/UnloadTexture raylib mechanic
class CTexture
{
public:
    CTexture(const std::string&& path)
    {
        tex = LoadTexture(path.c_str());
    }

    ~CTexture()
    {
        UnloadTexture(tex);
    }

    Texture Get() const
    {
        return tex;
    }

private:
    Texture tex;
};


// components

struct SpriteComponent
{
    // reference counting pointer so i can keep the texture alive
    // (it gets deleted when on constructed on stack)
    std::shared_ptr<CTexture> tex;
    
    Color tint = WHITE;
    struct Anchor {
        enum class AnchorType {
            TopLeft,    TopCenter,    TopRight,
            CenterLeft, Center,       CenterRight,
            BottomLeft, BottomCenter, BottomRight,

            Custom
        } anchor_type;

        // used if anchor_type is Custom
        Vector2 custom_anchor = { 0.f, 0.f };

        Vector2 ToVec(Vector2 dimensions) const
        {
            switch(anchor_type) {
                case AnchorType::TopLeft:
                    return { 0.f, 0.f };
                case AnchorType::TopCenter:
                    return { dimensions.x / 2.f, 0.f };
                case AnchorType::TopRight:
                    return { dimensions.x, 0.f };
                case AnchorType::CenterLeft:
                    return { 0.f, dimensions.y / 2.f };
                case AnchorType::Center:
                    return { dimensions.x / 2.f, dimensions.y / 2.f };
                case AnchorType::CenterRight:
                    return { dimensions.x, dimensions.y / 2.f };
                case AnchorType::BottomLeft:
                    return { 0.f, dimensions.y };
                case AnchorType::BottomCenter:
                    return { dimensions.x / 2.f, dimensions.y };
                case AnchorType::BottomRight:
                    return { dimensions.x, dimensions.y };
                case AnchorType::Custom:
                    return custom_anchor;
                default: return { 0.f, 0.f };
            };
        }
    } anchor = { Anchor::AnchorType::Center };
};

using TransformComponent = Transform;

struct SquishComponent
{
    Vector2 scale;
    float timer = 0.f;
    float frequency = 10.f;
};

// systems

void RenderSprites(const entt::registry& reg)
{
    auto view = reg.view<const TransformComponent, const SpriteComponent>();

    for (auto [entity, transform, sprite] : view.each())
    {
        auto tex = sprite.tex->Get();
        auto anchor_vec = sprite.anchor.ToVec({ float(tex.width), float(tex.height) });

        DrawTexturePro(
            tex, { 0.f, 0.f, float(tex.width), float(tex.height) }, // texture & size
            { transform.translation.x + anchor_vec.x * (1.f - transform.scale.x), transform.translation.y + anchor_vec.y * (1.f - transform.scale.y), // position & origin/anchor
              tex.width * transform.scale.x, tex.height * transform.scale.y }, // scale
            anchor_vec, // origin/anchor
            QuaternionToEuler(transform.rotation).x, sprite.tint // rotation & color tint
        );
    }
}

void SquishSprites(entt::registry& reg)
{
    const auto frequency = 10.f;
    
    auto view = reg.view<TransformComponent, const SpriteComponent, SquishComponent>();

    for (auto [entity, transform, sprite, squish] : view.each())
    {
        squish.timer += GetFrameTime();

        transform.scale.y = ((sinf(squish.timer * squish.frequency) / 4.5f) + .5f) * squish.scale.x;
        transform.scale.x = ((cosf(squish.timer * squish.frequency + sin(squish.timer / 2.f)) / 4.5f) + .5f) * squish.scale.y;

        if (squish.timer > PI * 2.f)
            squish.timer = 0.f;
    }

}

void FadeOut(entt::registry& reg)
{
    auto view = reg.view<SpriteComponent>();

    for (auto [entity, sprite] : view.each())
    {
        if (sprite.tint.a < 0.005f) {
            reg.destroy(entity);
        } 
        sprite.tint.a = Lerp(sprite.tint.a, 0.f, .025f);
    }
}

void SpawnKirb(entt::registry& registry, std::shared_ptr<CTexture> kirb_tex, Vector2 pos, Vector2 squish_scale, float squish_frequency)
{
    const auto kirb_img_path = "./kirb.png";
    auto entity = registry.create();

    registry.emplace<TransformComponent>(entity,
        Vector3 { pos.x, pos.y, 0.f },
        QuaternionIdentity(),
        Vector3 { 1.f, 1.f, 1.f });

    registry.emplace<SpriteComponent>(entity,
        kirb_tex,
        WHITE);

    registry.emplace<SquishComponent>(entity,
        squish_scale, 0.f, squish_frequency);
}


int main()
{
    const auto win_width = 1280;
    const auto win_height = 720;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(win_width, win_height, "hi");

    std::shared_ptr<CTexture> kirb_texture = std::make_shared<CTexture>("./kirb.png");

    entt::registry registry;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> pos_x(100, win_width - 100);
    std::uniform_int_distribution<> pos_y(100, win_height - 100);
    std::uniform_int_distribution<> scale_x(100, 200);
    std::uniform_int_distribution<> scale_y(100, 200);
    std::uniform_int_distribution<> frequency(5, 15);

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(BLACK);

            SquishSprites(registry);
            FadeOut(registry);
            RenderSprites(registry);

            // check for all keys
            for (int key = 65; key <= 90; ++key)
            {
                if (IsKeyPressed(key))
                    SpawnKirb(registry, kirb_texture,
                                        Vector2 { (float)pos_x(gen), (float)pos_y(gen) },
                                        Vector2 { (float)scale_x(gen) / 1000.f, (float)scale_y(gen) / 1000.f },
                                        frequency(gen));
            }
        EndDrawing();
    }

    CloseWindow();
}