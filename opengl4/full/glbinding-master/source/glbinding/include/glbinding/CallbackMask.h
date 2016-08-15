
#pragma once

#include <glbinding/glbinding_api.h>


namespace glbinding
{


/**
 * @brief
 *   The CallbackMask is a bitfield to encode the states of callbacks and logging for the OpenGL API function calls.
 */
enum class CallbackMask : unsigned char
{
    None        = 0, ///< All callbacks and logging is disabled.
    Unresolved  = 1 << 0, ///< Enables the callback for unresolved function calls.
    Before      = 1 << 1, ///< Enables the before callbacks.
    After       = 1 << 2, ///< Enables the after callbacks.
    Parameters  = 1 << 3, ///< Enables the provision of parameter values in the before and after callbacks.
    ReturnValue = 1 << 4, ///< Enables the provision of a return value in the after callback.
    Logging     = 1 << 5, ///< Enables logging to file.
    ParametersAndReturnValue = Parameters | ReturnValue,
    BeforeAndAfter = Before | After
};

/**
 * @brief
 *   External operator for bit-wise CallbackMask inverting.
 *
 * @param[in] a
 *   The CallbackMask to invert.
 *
 * @return
 *   The inverted CallbackMask.
 */
GLBINDING_API CallbackMask operator~(CallbackMask a);

/**
 * @brief
 *   External operator for bit-wise 'or' of CallbackMasks.
 *
 * @param[in] a
 *   The first CallbackMask.
 * @param[in] b
 *   The second CallbackMask.
 *
 * @return
 *   The compound CallbackMask.
 */
GLBINDING_API CallbackMask operator|(CallbackMask a, CallbackMask b);

/**
 * @brief
 *   External operator for bit-wise 'and' of CallbackMasks.
 *
 * @param[in] a
 *   The first CallbackMask.
 * @param[in] b
 *   The second CallbackMask.
 *
 * @return
 *   The compound CallbackMask.
 */
GLBINDING_API CallbackMask operator&(CallbackMask a, CallbackMask b);

/**
 * @brief
 *   External operator for bit-wise 'xor' of CallbackMasks.
 *
 * @param[in] a
 *   The first CallbackMask.
 * @param[in] b
 *   The second CallbackMask.
 *
 * @return
 *   The compound CallbackMask.
 */
GLBINDING_API CallbackMask operator^(CallbackMask a, CallbackMask b);

/**
 * @brief
 *   External operator for bit-wise 'or' assignment of CallbackMasks.
 *
 * @param[in] a
 *   The first CallbackMask.
 * @param[in] b
 *   The second CallbackMask.
 *
 * @return
 *   The first, now manipulated, CallbackMask.
 */
GLBINDING_API CallbackMask& operator|=(CallbackMask& a, CallbackMask b);

/**
 * @brief
 *   External operator for bit-wise 'and' assignment of CallbackMasks.
 *
 * @param[in] a
 *   The first CallbackMask.
 * @param[in] b
 *   The second CallbackMask.
 *
 * @return
 *   The first, now manipulated, CallbackMask.
 */
GLBINDING_API CallbackMask& operator&=(CallbackMask& a, CallbackMask b);

/**
 * @brief
 *   External operator for bit-wise 'xor' assignment of CallbackMasks.
 *
 * @param[in] a
 *   The first CallbackMask.
 * @param[in] b
 *   The second CallbackMask.
 *
 * @return
 *   The first, now manipulated, CallbackMask.
 */
GLBINDING_API CallbackMask& operator^=(CallbackMask& a, CallbackMask b);


} // namespace glbinding
