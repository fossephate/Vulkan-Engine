//
//  Created by Bradley Austin Davis on 2016/07/17
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef jherico_gltf_hpp
#define jherico_gltf_hpp

#include <string>
#include <glm/glm.hpp>

namespace gltf {
    using glm::ivec2;
    using glm::uvec2;
    using glm::vec2;
    using glm::vec3;
    using glm::vec4;
    using glm::mat3;
    using glm::mat4;
    using glm::quat;

    class root;

    namespace scenes {
        class scene;
        class node;
    }

    namespace meshes {
        class mesh;
    }

    namespace buffers {
        class buffer;
        class view;
        class accessor;
    }

    namespace shaders {
        class shader;
        class program;
    }

    namespace textures {
        class image;
    }

    namespace materials {
        class material;
    }

    namespace skins {

    }
}

#endif
