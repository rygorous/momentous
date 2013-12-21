#ifndef MATH_H_INCLUDED
#define MATH_H_INCLUDED

#include <cmath>

// Matrices are column-major.
//
// Coordinate system conventions:
//   +x = right
//   +y = down
//   +z = into screen
// this is a bit unorthodox but right-handed and convenient.

#define IMPL_COMPONENT_OP2(op) \
    this_type& operator op(const this_type& b) { x op b.x; y op b.y; return *this; }

#define IMPL_COMPONENT_OP3(op) \
    this_type& operator op(const this_type& b) { x op b.x; y op b.y; z op b.z; return *this; }

#define IMPL_COMPONENT_OP4(op) \
    this_type& operator op(const this_type& b) { x op b.x; y op b.y; z op b.z; w op b.w; return *this; }

#define IMPL_LINEAR_OPS(type) \
    template<typename T> type<T> operator -(const type<T>& v) { type<T> x = v; x *= T(-1); return x; } \
    template<typename T> type<T> operator +(const type<T>& a, const type<T>& b) { type<T> x = a; x += b; return x; } \
    template<typename T> type<T> operator -(const type<T>& a, const type<T>& b) { type<T> x = a; x -= b; return x; } \
    template<typename T> type<T> operator *(const type<T>& a, T s) { type<T> x = a; x *= s; return x; } \
    template<typename T> type<T> operator *(const type<T>& a, const type<T>& b) { type<T> x = a; x *= b; return x; } \
    template<typename T> type<T> operator *(T s, const type<T>& b) { type<T> x = b; x *= s; return x; }

#define IMPL_VECTOR_OPS(type, dot_expr) \
    IMPL_LINEAR_OPS(type) \
    template<typename T> T dot(const type<T>& a, const type<T>& b) { return dot_expr; } \
    template<typename T> T len_sq(const type<T>& a) { return dot(a, a); } \
    template<typename T> T len(const type<T>& a) { return std::sqrt(len_sq(a)); } \
    template<typename T> type<T> normalize(const type<T>& a) { return rsqrt(len_sq(a)) * a; }

#define IMPL_MATRIX_OPS(mat_type, vec_type, mul_expr) \
    IMPL_LINEAR_OPS(mat_type) \
    template<typename T> vec_type<T> operator *(const mat_type<T>& m, const vec_type<T>& v) { return mul_expr; }

namespace math {
    template<typename T>
    T rsqrt(T x)
    {
        return T(1) / std::sqrt(x);
    }

    template<typename T>
    struct vec2T {
        typedef vec2T<T> this_type;

        union {
            struct {
                T x, y;
            };
            T v[2];
        };

        vec2T() {}
        explicit vec2T(T s) : x(s), y(s) {}
        vec2T(T x, T y) : x(x), y(y) {}

        T operator[](int i) const { return v[i]; }
        T& operator[](int i) { return v[i]; }

        IMPL_COMPONENT_OP2(+=)
        IMPL_COMPONENT_OP2(-=)
        IMPL_COMPONENT_OP2(*=)
        this_type& operator *=(T s)                 { x *= s; y *= s; return *this; }
    };

    IMPL_VECTOR_OPS(vec2T, a.x*b.x + a.y*b.y)

    template<typename T>
    struct vec3T {
        typedef vec3T<T> this_type;

        union {
            struct {
                T x, y, z;
            };
            T v[3];
        };

        vec3T() {}
        explicit vec3T(T s) : x(s), y(s), z(s) {}
        vec3T(T x, T y, T z) : x(x), y(y), z(z) {}

        T operator[](int i) const { return v[i]; }
        T& operator[](int i) { return v[i]; }

        IMPL_COMPONENT_OP3(+=)
        IMPL_COMPONENT_OP3(-=)
        IMPL_COMPONENT_OP3(*=)
        this_type& operator *=(T s)                 { x *= s; y *= s; z *= s; return *this; }
    };

    IMPL_VECTOR_OPS(vec3T, a.x*b.x + a.y*b.y + a.z*b.z)
    template<typename T>
    vec3T<T> cross(const vec3T<T>& a, const vec3T<T>& b)
    {
        return vec3T<T>(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
    }

    template<typename T>
    struct vec4T {
        typedef vec4T<T> this_type;

        union {
            struct {
                T x, y, z, w;
            };
            T v[4];
        };

        vec4T() {}
        explicit vec4T(T s) : x(s), y(s), z(s), w(s) {}
        vec4T(const vec3T<T>& v, T w) : x(v.x), y(v.y), z(v.z), w(w) {}
        vec4T(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

        T operator[](int i) const { return v[i]; }
        T& operator[](int i) { return v[i]; }

        IMPL_COMPONENT_OP4(+=)
        IMPL_COMPONENT_OP4(-=)
        IMPL_COMPONENT_OP4(*=)
        this_type& operator *=(T s)                 { x *= s; y *= s; z *= s; w *= s; return *this; }
    };

    IMPL_VECTOR_OPS(vec4T, a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w)

    template<typename T>
    struct mat33T {
        typedef vec3T<T> vec_type;
        typedef mat33T<T> this_type;

        vec_type x, y, z; // columns

        mat33T() {}
        mat33T(const vec_type& colX, const vec_type& colY, const vec_type& colZ) : x(colX), y(colY), z(colZ) {}
        mat33T(
            T _00, T _01, T _02,
            T _10, T _11, T _12,
            T _20, T _21, T _22
            ) : x(_00, _10, _20), y(_01, _11, _21), z(_02, _12, _22) {}

        T operator()(int i, int j) const        { return (&x)[j][i]; }
        T& operator()(int i, int j)             { return (&x)[j][i]; }

        const vec_type& get_col(int i) const    { return (&x)[i]; }
        void set_col(int i, const vec_type& v)  { (&x)[i] = v; }
        const vec_type get_row(int i) const     { return vec_type((&x)[0][i], (&x)[1][i], (&x)[2][i]); }
        void set_row(int i, const vec_type& v)  { (&x)[0][i] = v.x; (&x)[1][i] = v.y; (&x)[2][i] = v.z; }

        IMPL_COMPONENT_OP3(+=)
        IMPL_COMPONENT_OP3(-=)
        this_type& operator *=(T s)             { x *= s; y *= s; z *= s; return *this; }
        this_type& operator *=(const this_type& b);

        static this_type diag(T x, T y, T z)    { return mat33T(x, T(0), T(0), T(0), y, T(0), T(0), T(0), z); }
        static this_type identity()             { return diag(T(1), T(1), T(1)); }
        static this_type uniform_scale(T s)     { return diag(s, s, s); }

        static this_type rotation(const vec_type& axis, T angle)
        {
            // Rodrigues rotation formula
            T cosv = std::cos(angle);
            vec_type sa = std::sin(angle) * axis;
            vec_type omca = (T(1) - cosv) * axis;

            return mat33T(
                omca.x*axis.x + cosv, omca.x*axis.y - sa.z, omca.x*axis.z + sa.y,
                omca.y*axis.x + sa.z, omca.y*axis.y + cosv, omca.y*axis.z - sa.x,
                omca.z*axis.x - sa.y, omca.z*axis.y + sa.x, omca.z*axis.z + cosv
            );
        }
    };

    IMPL_MATRIX_OPS(mat33T, vec3T, v.x*m.x + v.y*m.y + v.z*m.z)

    template<typename T>
    mat33T<T>& mat33T<T>::operator *=(const mat33T<T>& b)
    {
        const mat33T<T> M = *this;
        x = M * b.x;
        y = M * b.y;
        z = M * b.z;
        w = M * b.w;
        return *this;
    }

    template<typename T>
    mat33T<T> transpose(const mat33T<T>& m)
    {
        return mat33T<T>(m.get_row(0), m.get_row(1), m.get_row(2));
    }

    template<typename T>
    struct mat44T {
        typedef vec4T<T> vec_type;
        typedef vec3T<T> vec3_type;
        typedef mat44T<T> this_type;

        vec_type x, y, z, w; // columns

        mat44T() {}
        mat44T(const vec_type& colX, const vec_type& colY, const vec_type& colZ, const vec_type& colW) : x(colX), y(colY), z(colZ), w(colW) {}
        mat44T(const mat33T<T>& mat3x3, const vec3_type& translate) : x(mat3x3.x, T(0)), y(mat3x3.y, T(0)), z(mat3x3.z, T(0)), w(translate, T(1)) {}
        mat44T(
            T _00, T _01, T _02, T _03,
            T _10, T _11, T _12, T _13,
            T _20, T _21, T _22, T _23,
            T _30, T _31, T _32, T _33
            ) : x(_00, _10, _20, _30), y(_01, _11, _21, _31), z(_02, _12, _22, _32), w(_03, _13, _23, _33) {}

        T operator()(int i, int j) const        { return (&x)[j][i]; }
        T& operator()(int i, int j)             { return (&x)[j][i]; }

        const vec_type& get_col(int i) const    { return (&x)[i]; }
        void set_col(int i, const vec_type& v)  { (&x)[i] = v; }
        const vec_type get_row(int i) const     { return vec_type((&x)[0][i], (&x)[1][i], (&x)[2][i], (&x)[3][i]); }
        void set_row(int i, const vec_type& v)  { (&x)[0][i] = v.x; (&x)[1][i] = v.y; (&x)[2][i] = v.z; (&x)[3][i] = v.w; }

        IMPL_COMPONENT_OP4(+=)
        IMPL_COMPONENT_OP4(-=)
        this_type& operator *=(T s)             { x *= s; y *= s; z *= s; w *= s; return *this; }
        this_type& operator *=(const this_type& b);

        static this_type diag(T x, T y, T z, T w) { return mat44T(x, T(0), T(0), T(0), T(0), y, T(0), T(0), T(0), T(0), z, T(0), T(0), T(0), T(0), w); }
        static this_type identity()             { return diag(T(1), T(1), T(1), T(1)); }

        static this_type look_at(const vec3_type& pos, const vec3_type& look_at, const vec3_type& down)
        {
            mat33T<T> M;
            vec3_type z_axis = normalize(look_at - pos);
            vec3_type x_axis = normalize(cross(down, z_axis));
            vec3_type y_axis = cross(z_axis, x_axis);

            M.set_row(0, x_axis);
            M.set_row(1, y_axis);
            M.set_row(2, z_axis);
            return this_type(M, M * -pos);
        }

        static this_type orthoD3D(T lft, T rgt, T top, T bot, T nearv, T farv)
        {
            vec3_type mid((lft + rgt) / T(2), (bot + top) / T(2), (nearv + farv) / T(2));
            T sx = T(2) / (rgt - lft);
            T sy = T(2) / (top - bot);
            T sz = T(1) / (farv - nearv);

            return this_type(mat33T<T>::diag(sx, sy, sz), vec3T<T>(-mid.x * sx, -mid.y * sy, T(0.5) - mid.z * sz));
        }

        // NOTE: this takes lft/rgt/bot/top at z=1 plane, not near plane!
        static this_type frustumD3D(T lft, T rgt, T top, T bot, T nearv, T farv)
        {
            T Q = farv / (farv - nearv);

            return this_type(
                T(2) / (rgt - lft), 0,                  (rgt + lft) / (rgt - lft), T(0),
                T(0),               T(2) / (top - bot), (top + bot) / (top - bot), T(0),
                T(0),               T(0),               Q,                         -nearv * Q,
                T(0),               T(0),               T(1),                      T(0)
            );
        }

        // w/h at z=1 plane, not near plane!
        static this_type perspectiveD3D(T w, T h, T nearv, T farv)
        {
            T wh = w / T(2);
            T hh = h / T(2);
            return frustumD3D(-wh, wh, -hh, hh, nearv, farv);
        }
    };

    IMPL_MATRIX_OPS(mat44T, vec4T, v.x*m.x + v.y*m.y + v.z*m.z + v.w*m.w)

    template<typename T>
    mat44T<T>& mat44T<T>::operator *=(const mat44T<T>& b)
    {
        const mat44T<T> M = *this;
        x = M * b.x;
        y = M * b.y;
        z = M * b.z;
        w = M * b.w;
        return *this;
    }

    template<typename T>
    mat44T<T> transpose(const mat44T<T>& m)
    {
        return mat44T<T>(m.get_row(0), m.get_row(1), m.get_row(2), m.get_row(3));
    }

    typedef vec2T<int> vec2i;
    typedef vec2T<float> vec2;

    typedef vec3T<int> vec3i;
    typedef vec3T<float> vec3;

    typedef vec4T<int> vec4i;
    typedef vec4T<float> vec4;

    typedef mat33T<float> mat33;

    typedef mat44T<float> mat44;
}

#undef IMPL_COMPONENT_OP2
#undef IMPL_COMPONENT_OP3
#undef IMPL_COMPONENT_OP4

#undef IMPL_LINEAR_OPS
#undef IMPL_VECTOR_OPS

#endif // MATH_H_INCLUDED