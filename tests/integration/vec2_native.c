/* C implementation for native struct Vec2 as ref method chaining test.
 * The struct layout must match what the Sindarin compiler generates. */

typedef struct {
    long long x;
    long long y;
} __sn__Vec2;

__sn__Vec2 rt_vec2_add(__sn__Vec2 *self, __sn__Vec2 other)
{
    __sn__Vec2 result = { self->x + other.x, self->y + other.y };
    return result;
}

__sn__Vec2 rt_vec2_scale(__sn__Vec2 *self, long long factor)
{
    __sn__Vec2 result = { self->x * factor, self->y * factor };
    return result;
}

__sn__Vec2 rt_vec2_negate(__sn__Vec2 *self)
{
    __sn__Vec2 result = { -(self->x), -(self->y) };
    return result;
}
