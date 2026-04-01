#pragma once

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/// Simple 1-pole IIR parameter smoother.
/// Realtime-safe: no heap allocations, no mutex, no I/O, no exceptions.
///
/// Formula per sample: mCurrent += mCoefficient * (mTarget - mCurrent)
///
/// Default coefficient: 1.0 - exp(-2*pi*cutoffHz / sampleRate)
/// with cutoffHz ≈ 10–20 Hz gives a smooth ~16–32ms glide.
class ParamSmoother
{
public:
  ParamSmoother() : mCurrent(0.0), mTarget(0.0), mCoefficient(0.1) {}

  /// Compute the default smoothing coefficient for a given cutoff and sample rate.
  static double makeCoefficient(double cutoffHz, double sampleRate)
  {
    if (sampleRate <= 0.0) return 1.0;
    const double arg = -2.0 * M_PI * cutoffHz / sampleRate;
    return 1.0 - std::exp(arg);
  }

  /// Set the smoothing coefficient directly (0 = no change, 1 = instant).
  void setCoefficient(double coeff)
  {
    if (coeff < 0.0) coeff = 0.0;
    if (coeff > 1.0) coeff = 1.0;
    mCoefficient = coeff;
  }

  /// Set the target value (called from any thread, atomically safe for doubles on x86/ARM).
  void setTarget(double target) { mTarget = target; }

  /// Immediately jump to a value (bypasses smoothing).
  void reset(double value)
  {
    mCurrent = value;
    mTarget  = value;
  }

  /// Advance one sample and return the current smoothed value.
  double process()
  {
    mCurrent += mCoefficient * (mTarget - mCurrent);
    return mCurrent;
  }

  /// Return the current smoothed value without advancing.
  double getCurrent() const { return mCurrent; }

  /// Return the target value.
  double getTarget() const { return mTarget; }

private:
  double mCurrent;
  double mTarget;
  double mCoefficient;
};
