#pragma once

#include <glbinding/no{{api}}.h>


namespace {{api}}
{


enum class GLboolean : unsigned char
{
{{#booleans.items}}
    {{item.identifier}} = {{item.value}}{{^last}},{{/last}}
{{/booleans.items}}
};

// import booleans to namespace

{{#booleans.items}}
static const GLboolean {{item.identifier}} = GLboolean::{{item.identifier}};
{{/booleans.items}}


} // namespace {{api}}
