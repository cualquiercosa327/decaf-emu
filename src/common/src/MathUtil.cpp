// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <cmath>
#include <cstring>
#include <limits>
#include <numeric>

#include "Common/MathUtil.h"

namespace MathUtil
{
uint32_t ClassifyDouble(double dvalue)
{
  // TODO: Optimize the below to be as fast as possible.
  IntDouble value(dvalue);
  uint64_t sign = value.i & DOUBLE_SIGN;
  uint64_t exp = value.i & DOUBLE_EXP;
  if (exp > DOUBLE_ZERO && exp < DOUBLE_EXP)
  {
    // Nice normalized number.
    return sign ? PPC_FPCLASS_NN : PPC_FPCLASS_PN;
  }
  else
  {
    uint64_t mantissa = value.i & DOUBLE_FRAC;
    if (mantissa)
    {
      if (exp)
      {
        return PPC_FPCLASS_QNAN;
      }
      else
      {
        // Denormalized number.
        return sign ? PPC_FPCLASS_ND : PPC_FPCLASS_PD;
      }
    }
    else if (exp)
    {
      // Infinite
      return sign ? PPC_FPCLASS_NINF : PPC_FPCLASS_PINF;
    }
    else
    {
      // Zero
      return sign ? PPC_FPCLASS_NZ : PPC_FPCLASS_PZ;
    }
  }
}

uint32_t ClassifyFloat(float fvalue)
{
  // TODO: Optimize the below to be as fast as possible.
  IntFloat value(fvalue);
  uint32_t sign = value.i & FLOAT_SIGN;
  uint32_t exp = value.i & FLOAT_EXP;
  if (exp > FLOAT_ZERO && exp < FLOAT_EXP)
  {
    // Nice normalized number.
    return sign ? PPC_FPCLASS_NN : PPC_FPCLASS_PN;
  }
  else
  {
    uint32_t mantissa = value.i & FLOAT_FRAC;
    if (mantissa)
    {
      if (exp)
      {
        return PPC_FPCLASS_QNAN;  // Quiet NAN
      }
      else
      {
        // Denormalized number.
        return sign ? PPC_FPCLASS_ND : PPC_FPCLASS_PD;
      }
    }
    else if (exp)
    {
      // Infinite
      return sign ? PPC_FPCLASS_NINF : PPC_FPCLASS_PINF;
    }
    else
    {
      // Zero
      return sign ? PPC_FPCLASS_NZ : PPC_FPCLASS_PZ;
    }
  }
}

const int frsqrte_expected_base[] = {
    0x3ffa000, 0x3c29000, 0x38aa000, 0x3572000, 0x3279000, 0x2fb7000, 0x2d26000, 0x2ac0000,
    0x2881000, 0x2665000, 0x2468000, 0x2287000, 0x20c1000, 0x1f12000, 0x1d79000, 0x1bf4000,
    0x1a7e800, 0x17cb800, 0x1552800, 0x130c000, 0x10f2000, 0x0eff000, 0x0d2e000, 0x0b7c000,
    0x09e5000, 0x0867000, 0x06ff000, 0x05ab800, 0x046a000, 0x0339800, 0x0218800, 0x0105800,
};
const int frsqrte_expected_dec[] = {
    0x7a4, 0x700, 0x670, 0x5f2, 0x584, 0x524, 0x4cc, 0x47e, 0x43a, 0x3fa, 0x3c2,
    0x38e, 0x35e, 0x332, 0x30a, 0x2e6, 0x568, 0x4f3, 0x48d, 0x435, 0x3e7, 0x3a2,
    0x365, 0x32e, 0x2fc, 0x2d0, 0x2a8, 0x283, 0x261, 0x243, 0x226, 0x20b,
};

double ApproximateReciprocalSquareRoot(double val)
{
  union
  {
    double valf;
    int64_t vali;
  };
  valf = val;
  int64_t mantissa = vali & ((1LL << 52) - 1);
  int64_t sign = vali & (1ULL << 63);
  int64_t exponent = vali & (0x7FFLL << 52);

  // Special case 0
  if (mantissa == 0 && exponent == 0)
    return sign ? -std::numeric_limits<double>::infinity() :
                  std::numeric_limits<double>::infinity();
  // Special case NaN-ish numbers
  if (exponent == (0x7FFLL << 52))
  {
    if (mantissa == 0)
    {
      if (sign)
        return std::numeric_limits<double>::quiet_NaN();

      return 0.0;
    }

    return 0.0 + valf;
  }

  // Negative numbers return NaN
  if (sign)
    return std::numeric_limits<double>::quiet_NaN();

  if (!exponent)
  {
    // "Normalize" denormal values
    do
    {
      exponent -= 1LL << 52;
      mantissa <<= 1;
    } while (!(mantissa & (1LL << 52)));
    mantissa &= (1LL << 52) - 1;
    exponent += 1LL << 52;
  }

  bool odd_exponent = !(exponent & (1LL << 52));
  exponent = ((0x3FFLL << 52) - ((exponent - (0x3FELL << 52)) / 2)) & (0x7FFLL << 52);

  int i = (int)(mantissa >> 37);
  vali = sign | exponent;
  int index = i / 2048 + (odd_exponent ? 16 : 0);
  vali |= (int64_t)(frsqrte_expected_base[index] - frsqrte_expected_dec[index] * (i % 2048)) << 26;
  return valf;
}

const int fres_expected_base[] = {
    0x7ff800, 0x783800, 0x70ea00, 0x6a0800, 0x638800, 0x5d6200, 0x579000, 0x520800,
    0x4cc800, 0x47ca00, 0x430800, 0x3e8000, 0x3a2c00, 0x360800, 0x321400, 0x2e4a00,
    0x2aa800, 0x272c00, 0x23d600, 0x209e00, 0x1d8800, 0x1a9000, 0x17ae00, 0x14f800,
    0x124400, 0x0fbe00, 0x0d3800, 0x0ade00, 0x088400, 0x065000, 0x041c00, 0x020c00,
};
const int fres_expected_dec[] = {
    0x3e1, 0x3a7, 0x371, 0x340, 0x313, 0x2ea, 0x2c4, 0x2a0, 0x27f, 0x261, 0x245,
    0x22a, 0x212, 0x1fb, 0x1e5, 0x1d1, 0x1be, 0x1ac, 0x19b, 0x18b, 0x17c, 0x16e,
    0x15b, 0x15b, 0x143, 0x143, 0x12d, 0x12d, 0x11a, 0x11a, 0x108, 0x106,
};

// Used by fres and ps_res.
double ApproximateReciprocal(double val)
{
  // We are using namespace std scoped here because the Android NDK is complete trash as usual
  // For 32bit targets(mips, ARMv7, x86) it doesn't provide an implementation of std::copysign
  // but instead provides just global namespace copysign implementations.
  // The workaround for this is to just use namespace std within this function's scope
  // That way on real toolchains it will use the std:: variant like normal.
  using namespace std;
  union
  {
    double valf;
    int64_t vali;
  };

  valf = val;
  int64_t mantissa = vali & ((1LL << 52) - 1);
  int64_t sign = vali & (1ULL << 63);
  int64_t exponent = vali & (0x7FFLL << 52);

  // Special case 0
  if (mantissa == 0 && exponent == 0)
    return copysign(std::numeric_limits<double>::infinity(), valf);

  // Special case NaN-ish numbers
  if (exponent == (0x7FFLL << 52))
  {
    if (mantissa == 0)
      return copysign(0.0, valf);
    return 0.0 + valf;
  }

  // Special case small inputs
  if (exponent < (895LL << 52))
    return copysign(std::numeric_limits<float>::max(), valf);

  // Special case large inputs
  if (exponent >= (1149LL << 52))
    return copysign(0.0, valf);

  exponent = (0x7FDLL << 52) - exponent;

  int i = (int)(mantissa >> 37);
  vali = sign | exponent;
  vali |= (int64_t)(fres_expected_base[i / 1024] - (fres_expected_dec[i / 1024] * (i % 1024) + 1) / 2)
          << 29;
  return valf;
}

}  // namespace

inline void MatrixMul(int n, const float* a, const float* b, float* result)
{
  for (int i = 0; i < n; ++i)
  {
    for (int j = 0; j < n; ++j)
    {
      float temp = 0;
      for (int k = 0; k < n; ++k)
      {
        temp += a[i * n + k] * b[k * n + j];
      }
      result[i * n + j] = temp;
    }
  }
}

// Calculate sum of a float list
float MathFloatVectorSum(const std::vector<float>& Vec)
{
  return std::accumulate(Vec.begin(), Vec.end(), 0.0f);
}

void Matrix33::LoadIdentity(Matrix33& mtx)
{
  memset(mtx.data, 0, sizeof(mtx.data));
  mtx.data[0] = 1.0f;
  mtx.data[4] = 1.0f;
  mtx.data[8] = 1.0f;
}

void Matrix33::LoadQuaternion(Matrix33& mtx, const Quaternion& quat)
{
  const float qw = quat.data[0], qx = quat.data[1], qy = quat.data[2], qz = quat.data[3];
  const float ww = qw * qw, xx = qx * qx, yy = qy * qy, zz = qz * qz;

  mtx.data[0] = ww + xx - yy - zz;
  mtx.data[1] = 2.0f * (qx * qy - qw * qz);
  mtx.data[2] = 2.0f * (qx * qz + qw * qy);

  mtx.data[3] = 2.0f * (qx * qy + qw * qz);
  mtx.data[4] = ww - xx + yy - zz;
  mtx.data[5] = 2.0f * (qy * qz - qw * qx);

  mtx.data[6] = 2.0f * (qx * qz - qw * qy);
  mtx.data[7] = 2.0f * (qy * qz + qw * qx);
  mtx.data[8] = ww - xx - yy + zz;
}

void Matrix33::RotateX(Matrix33& mtx, float rad)
{
  float s = sin(rad);
  float c = cos(rad);
  memset(mtx.data, 0, sizeof(mtx.data));
  mtx.data[0] = 1;
  mtx.data[4] = c;
  mtx.data[5] = -s;
  mtx.data[7] = s;
  mtx.data[8] = c;
}
void Matrix33::RotateY(Matrix33& mtx, float rad)
{
  float s = sin(rad);
  float c = cos(rad);
  memset(mtx.data, 0, sizeof(mtx.data));
  mtx.data[0] = c;
  mtx.data[2] = s;
  mtx.data[4] = 1;
  mtx.data[6] = -s;
  mtx.data[8] = c;
}
// VR roll
void Matrix33::RotateZ(Matrix33& mtx, float rad)
{
  float s = sin(rad);
  float c = cos(rad);
  memset(mtx.data, 0, sizeof(mtx.data));
  mtx.data[0] = c;
  mtx.data[1] = -s;
  mtx.data[3] = s;
  mtx.data[4] = c;
  mtx.data[8] = 1;
}

void Matrix33::Multiply(const Matrix33& a, const Matrix33& b, Matrix33& result)
{
  MatrixMul(3, a.data, b.data, result.data);
}

void Matrix33::Multiply(const Matrix33& a, const float vec[3], float result[3])
{
  for (int i = 0; i < 3; ++i)
  {
    result[i] = 0;

    for (int k = 0; k < 3; ++k)
    {
      result[i] += a.data[i * 3 + k] * vec[k];
    }
  }
}

Matrix33 Matrix33::operator*(const Matrix33& rhs) const
{
  Matrix33 result;
  MatrixMul(3, rhs.data, this->data, result.data);
  return result;
}

// GlovePIE function for extracting yaw, pitch, and roll from a rotation matrix
void Matrix33::GetPieYawPitchRollR(const Matrix33& m, float& yaw, float& pitch, float& roll)
{
  float s, c, cp;
  pitch = asin(m.data[2 * 3 + 1]);
  cp = cos(pitch);

  // yaw:=arcsin(m[2][0]/cp);
  s = m.data[2 * 3 + 0] / cp;
  c = m.data[2 * 3 + 2] / cp;
  yaw = atan2(s, c);

  s = -m.data[0 * 3 + 1] / cp;
  c = m.data[1 * 3 + 1] / cp;
  roll = atan2(s, c);
}

void Matrix44::LoadIdentity(Matrix44& mtx)
{
  memset(mtx.data, 0, sizeof(mtx.data));
  mtx.data[0] = 1.0f;
  mtx.data[5] = 1.0f;
  mtx.data[10] = 1.0f;
  mtx.data[15] = 1.0f;
}

void Matrix44::LoadMatrix33(Matrix44& mtx, const Matrix33& m33)
{
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      mtx.data[i * 4 + j] = m33.data[i * 3 + j];
    }
  }

  for (int i = 0; i < 3; ++i)
  {
    mtx.data[i * 4 + 3] = 0;
    mtx.data[i + 12] = 0;
  }
  mtx.data[15] = 1.0f;
}

Matrix44& Matrix44::operator=(const Matrix33& rhs)
{
  LoadMatrix33(*this, rhs);
  return *this;
}

void Matrix44::Set(Matrix44& mtx, const float mtxArray[16])
{
  for (int i = 0; i < 16; ++i)
  {
    mtx.data[i] = mtxArray[i];
  }
}

void Matrix44::Set3x4(Matrix44 & mtx, const float mtxArray[12])
{
   for (int i = 0; i < 12; ++i)
   {
      mtx.data[i] = mtxArray[i];
   }
   memset(&mtx.data[12], 0, sizeof(mtx.data[12]) * 3);
   mtx.data[15] = 1;
}

void Matrix44::Translate(Matrix44& mtx, const float vec[3])
{
  LoadIdentity(mtx);
  mtx.data[3] = vec[0];
  mtx.data[7] = vec[1];
  mtx.data[11] = vec[2];
}

void Matrix44::InvertTranslation(Matrix44& mtx)
{
  mtx.data[3] = -mtx.data[3];
  mtx.data[7] = -mtx.data[7];
  mtx.data[11] = -mtx.data[11];
}

void Matrix44::InvertScale(Matrix44& mtx)
{
  mtx.data[0] = 1 / mtx.data[0];
  mtx.data[1 * 4 + 1] = 1 / mtx.data[1 * 4 + 1];
  mtx.data[2 * 4 + 2] = 1 / mtx.data[2 * 4 + 2];
}

void Matrix44::InvertRotation(Matrix44& mtx)
{
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < r; ++c)
    {
      float temp = mtx.data[r * 4 + c];
      mtx.data[r * 4 + c] = mtx.data[c * 4 + r];
      mtx.data[c * 4 + r] = temp;
    }
}

// from PPSSPP
Matrix44 Matrix44::inverse() const
{
  Matrix44 temp;
  float dW =
      1.0f / (xx * (yy * zz - yz * zy) - xy * (yx * zz - yz * zx) - xz * (yy * zx - yx * zy));

  temp.xx = (yy * zz - yz * zy) * dW;
  temp.xy = (xz * zy - xy * zz) * dW;
  temp.xz = (xy * yz - xz * yy) * dW;
  temp.xw = xw;

  temp.yx = (yz * zx - yx * zz) * dW;
  temp.yy = (xx * zz - xz * zx) * dW;
  temp.yz = (xz * yx - xx * zx) * dW;
  temp.yw = yw;

  temp.zx = (yx * zy - yy * zx) * dW;
  temp.zy = (xy * zx - xx * zy) * dW;
  temp.zz = (xx * yy - xy * yx) * dW;
  temp.zw = zw;

  temp.wx = (yy * (zx * wz - zz * wx) + yz * (zy * wx - zx * wy) - yx * (zy * wz - zz * wy)) * dW;
  temp.wy = (xx * (zy * wz - zz * wy) + xy * (zz * wx - zx * wz) + xz * (zx * wy - zy * wx)) * dW;
  temp.wz = (xy * (yx * wz - yz * wx) + xz * (yy * wx - yx * wy) - xx * (yy * wz - yz * wy)) * dW;
  temp.ww = ww;

  return temp;
}

// from PPSSPP - assumes there's no scale, only rotation and translation
Matrix44 Matrix44::simpleInverse() const
{
  Matrix44 out;
  out.xx = xx;
  out.xy = yx;
  out.xz = zx;

  out.yx = xy;
  out.yy = yy;
  out.yz = zy;

  out.zx = xz;
  out.zy = yz;
  out.zz = zz;

  out.wx = -(xx * wx + xy * wy + xz * wz);
  out.wy = -(yx * wx + yy * wy + yz * wz);
  out.wz = -(zx * wx + zy * wy + zz * wz);

  out.xw = 0.0f;
  out.yw = 0.0f;
  out.zw = 0.0f;
  out.ww = 1.0f;

  return out;
}

Matrix44 Matrix44::transpose() const
{
  Matrix44 out;
  out.xx = xx;
  out.xy = yx;
  out.xz = zx;
  out.xw = wx;
  out.yx = xy;
  out.yy = yy;
  out.yz = zy;
  out.yw = wy;
  out.zx = xz;
  out.zy = yz;
  out.zz = zz;
  out.zw = wz;
  out.wx = xw;
  out.wy = yw;
  out.wz = zw;
  out.ww = ww;
  return out;
}

Matrix44 Matrix44::extractOpenGLProjection() const
{
   Matrix44 out;
   out.empty();
   out.xx = sqrt(xx*xx + yx*yx + zx*zx);
   out.yy = sqrt(xy*xy + yy*yy + zy*zy);
   float a;
   if (xw != 0) a = -xz / xw;
   else if (yw != 0) a = -yz / yw;
   else a = -zz / zw;
   out.zz = a;
   out.wz = wz + a * ww;
   out.zw = -1;
   return out;
}

Matrix44 Matrix44::extractOpenGLView() const
{
   Matrix44 out;
   float h = sqrt(xx*xx + yx*yx + zx*zx);
   float v = sqrt(xy*xy + yy*yy + zy*zy);
   for (int r = 0; r < 4; r++)
   {
      out.data[0 * 4 + r] = data[0 * 4 + r] / h;
      out.data[1 * 4 + r] = data[1 * 4 + r] / v;
      out.data[2 * 4 + r] = -data[3 * 4 + r];
      out.data[3 * 4 + r] = 0;
   }
   out.ww = 1;
   return out;
}

float Matrix44::simpleHFOV() const
{
   return 2 * atan(1.0f / xx) * 180.0f / 3.1415926535f;
}

float Matrix44::hFOV() const
{
   float x = sqrt(xx*xx + yx*yx + zx*zx);
   return 2 * atan(1.0f / x) * 180.0f / 3.1415926535f;
}

float Matrix44::simpleVFOV() const
{
   return 2 * atan(1.0f / yy) * 180.0f / 3.1415926535f;
}

float Matrix44::vFOV() const
{
   float y = sqrt(xy*xy + yy*yy + zy*zy);
   return 2 * atan(1.0f / y) * 180.0f / 3.1415926535f;
}

float Matrix44::simpleAspectRatio() const
{
   return yy / xx;
}

float Matrix44::aspectRatio() const
{
   return sqrt(xy*xy + yy*yy + zy*zy) / sqrt(xx*xx + yx*yx + zx*zx);
}

float Matrix44::openglFar() const
{
   return wz / (zz + 1);
}

float Matrix44::openglNear() const
{
   return wz / (zz - 1);
}

void Matrix44::Shear(Matrix44& mtx, const float a, const float b)
{
  LoadIdentity(mtx);
  mtx.data[2] = a;
  mtx.data[6] = b;
}
void Matrix44::Scale(Matrix44& mtx, const float vec[3])
{
  LoadIdentity(mtx);
  mtx.data[0] = vec[0];
  mtx.data[5] = vec[1];
  mtx.data[10] = vec[2];
}

void Matrix44::Multiply(const Matrix44& a, const Matrix44& b, Matrix44& result)
{
  MatrixMul(4, a.data, b.data, result.data);
}

void Matrix44::Multiply(const Matrix44& a, const float vec[3], float result[3])
{
  float v[4], r[4];
  v[0] = vec[0];
  v[1] = vec[1];
  v[2] = vec[2];
  v[3] = 1.0;
  for (int i = 0; i < 4; ++i)
  {
    r[i] = 0;
    for (int k = 0; k < 4; ++k)
    {
      r[i] += a.data[i * 4 + k] * v[k];
    }
  }
  for (int i = 0; i < 3; ++i)
    result[i] = r[i];
}

Matrix44 Matrix44::operator*(const Matrix44& rhs) const
{
  Matrix44 result;
  MatrixMul(4, rhs.data, this->data, result.data);
  return result;
}

void Quaternion::LoadIdentity(Quaternion& quat)
{
  quat.data[0] = 1.0f;
  quat.data[1] = 0.0f;
  quat.data[2] = 0.0f;
  quat.data[3] = 0.0f;
}

void Quaternion::Set(Quaternion& quat, const float quatArray[4])
{
  for (int i = 0; i < 4; ++i)
  {
    quat.data[i] = quatArray[i];
  }
}

void Quaternion::Invert(Quaternion& quat)
{
  quat.data[1] *= -1;
  quat.data[2] *= -1;
  quat.data[3] *= -1;
}

void Quaternion::Multiply(const Quaternion& a, const Quaternion& b, Quaternion& result)
{
  const float aw = a.data[0], ax = a.data[1], ay = a.data[2], az = a.data[3];
  const float bw = b.data[0], bx = b.data[1], by = b.data[2], bz = b.data[3];

  result.data[0] = aw * bw - ax * bx - ay * by - az * bz;
  result.data[1] = aw * bx + ax * bw + ay * bz - az * by;
  result.data[2] = aw * by - ax * bz + ay * bw + az * bx;
  result.data[3] = aw * bz + ax * by - ay * bx + az * bw;
}