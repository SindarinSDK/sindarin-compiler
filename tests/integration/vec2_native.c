/* C implementation for native struct Vec2 as ref method chaining test.
 * The struct __sn__Vec2 is defined by the generated code.
 * For 'as ref' structs, methods receive and return heap-allocated pointers. */

#include <stdlib.h>

__sn__Vec2 *__sn__Vec2_add(__sn__Vec2 *self, __sn__Vec2 *other)
{
    __sn__Vec2 *result = __sn__Vec2__new();
    result->__sn__x = self->__sn__x + other->__sn__x;
    result->__sn__y = self->__sn__y + other->__sn__y;
    return result;
}

__sn__Vec2 *__sn__Vec2_scale(__sn__Vec2 *self, long long factor)
{
    __sn__Vec2 *result = __sn__Vec2__new();
    result->__sn__x = self->__sn__x * factor;
    result->__sn__y = self->__sn__y * factor;
    return result;
}

__sn__Vec2 *__sn__Vec2_negate(__sn__Vec2 *self)
{
    __sn__Vec2 *result = __sn__Vec2__new();
    result->__sn__x = -(self->__sn__x);
    result->__sn__y = -(self->__sn__y);
    return result;
}
