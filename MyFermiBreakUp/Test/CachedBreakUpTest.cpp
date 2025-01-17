//
// Created by Artem Novikov on 24.05.2023.
//

#include <gtest/gtest.h>
#include <CLHEP/Units/PhysicalConstants.h>

#include "FermiBreakUp.h"
#include "Utilities/NucleiProperties/NucleiProperties.h"
#include "Utilities/Randomizer.h"
#include "CachedFermiConfigurations.h"

using namespace properties;

float CachedCalculateFragmentCount(MassNumber mass, ChargeNumber charge, const Vector3& vec,
                                   FermiFloat energy, size_t tests) {
  auto model = FermiBreakUp(std::make_unique<CachedFermiConfigurations>());
  size_t parts_counter = 0;
  auto additional_energy = energy * FermiFloat(mass);
  auto mom = LorentzVector(vec, std::sqrt(vec.mag2() +
      std::pow(NucleiProperties().GetNuclearMass(mass, charge) + additional_energy, 2)));
  auto particle = FermiParticle(mass, charge, mom);
  for (size_t i = 0; i < tests; ++i) {
    auto particles = model.BreakItUp(particle);
    parts_counter += particles.size();
  }
  return float(parts_counter) / tests;
}

TEST(FermiBreakUpTests, CachedCarbonTest) {
  size_t runs = 1e3;
  /// Carbons shouldn't break up [0, 0.7]
  ASSERT_NEAR(1, CachedCalculateFragmentCount(12_m, 6_c, {0, 0, 0}, 0 * CLHEP::MeV, runs), 0.01);
  ASSERT_NEAR(1, CachedCalculateFragmentCount(12_m, 6_c, {0, 0, 0}, 0.25 * CLHEP::MeV, runs), 0.01);
  ASSERT_NEAR(1, CachedCalculateFragmentCount(12_m, 6_c, {0, 0, 0}, 0.5 * CLHEP::MeV, runs), 0.01);
  ASSERT_NEAR(1, CachedCalculateFragmentCount(12_m, 6_c, {0, 0, 0}, 0.7 * CLHEP::MeV, runs), 0.01);

  /// Carbons should break into 3 parts [1, 1.4]
  ASSERT_NEAR(3, CachedCalculateFragmentCount(12_m, 6_c, {0, 0, 0}, 1 * CLHEP::MeV, runs), 0.01);
  ASSERT_NEAR(3, CachedCalculateFragmentCount(12_m, 6_c, {0, 0, 0}, 1.25 * CLHEP::MeV, runs), 0.01);
  ASSERT_NEAR(3, CachedCalculateFragmentCount(12_m, 6_c, {0, 0, 0}, 1.4 * CLHEP::MeV, runs), 0.01);


  /// Carbons should break into less than 3 parts [1.5, 3.5]
  ASSERT_LE(CachedCalculateFragmentCount(12_m, 6_c, {0, 0, 0}, 1.5 * CLHEP::MeV, runs), 3);
  ASSERT_LE(CachedCalculateFragmentCount(12_m, 6_c, {0, 0, 0}, 2 * CLHEP::MeV, runs), 3);
  ASSERT_LE(CachedCalculateFragmentCount(12_m, 6_c, {0, 0, 0}, 3 * CLHEP::MeV, runs), 3);
  ASSERT_LE(CachedCalculateFragmentCount(12_m, 6_c, {0, 0, 0}, 3.5 * CLHEP::MeV, runs), 3);

  /// Carbons should break into more than 3 parts [5, ...]
  ASSERT_GE(CachedCalculateFragmentCount(12_m, 6_c, {0, 0, 0}, 5 * CLHEP::MeV, runs), 3);
  ASSERT_GE(CachedCalculateFragmentCount(12_m, 6_c, {0, 0, 0}, 7 * CLHEP::MeV, runs), 3);
  ASSERT_GE(CachedCalculateFragmentCount(12_m, 6_c, {0, 0, 0}, 9 * CLHEP::MeV, runs), 3);
  ASSERT_GE(CachedCalculateFragmentCount(12_m, 6_c, {0, 0, 0}, 20 * CLHEP::MeV, runs), 3);
}

TEST(FermiBreakUpTests, CachedUnstableNucleiTest) {
  size_t runs = 1e3;

  /// protons should break up
  for (int i = 2; i <= 16; ++i) {
    ASSERT_NEAR(i, CachedCalculateFragmentCount(MassNumber(i), ChargeNumber(i), {0, 0, 0}, 0, runs), 0.01);
  }

  /// neutron should break up
  for (int i = 2; i <= 16; ++i) {
    ASSERT_NEAR(i, CachedCalculateFragmentCount(MassNumber(i), 0_c, {0, 0, 0}, 0, runs), 0.01);
  }
}

TEST(FermiBreakUpTests, CachedMomentumConservation) {
  auto model = FermiBreakUp();
  int seed = 5;
  srand(seed);
  int tries = 500;
  size_t runs = 1e2;
  for (int t = 0; t < tries; ++t) {
    MassNumber mass(rand() % 16 + 1);
    ChargeNumber charge(rand() % (int(mass) + 1));
    FermiFloat energy = (rand() % 1000) * CLHEP::MeV * FermiFloat(mass);
    auto vec = Randomizer::IsotropicVector() * (rand() % 1000) * CLHEP::MeV;
    auto mom = LorentzVector(vec, std::sqrt(std::pow(NucleiProperties().GetNuclearMass(mass, charge) + energy, 2) +
        vec.mag2()));
    auto particle = FermiParticle(mass, charge, mom);
    for (size_t i = 0; i < runs; ++i) {
      LorentzVector sum(0, 0, 0, 0);
      auto particles = model.BreakItUp(particle);
      for (auto& fragment : particles) {
        sum += fragment.GetMomentum();
      }
      ASSERT_NEAR(sum.x(), mom.x(), 1e-5);
      ASSERT_NEAR(sum.y(), mom.y(), 1e-5);
      ASSERT_NEAR(sum.z(), mom.z(), 1e-5);
      ASSERT_NEAR(sum.e(), mom.e(), 1e-5);
    }
  }
}

TEST(FermiBreakUpTests, CachedBaryonConservation) {
  auto model = FermiBreakUp();
  int seed = 5;
  srand(seed);
  int tries = 500;
  size_t runs = 1e2;
  for (int t = 0; t < tries; ++t) {
    MassNumber mass(rand() % 16 + 1);
    ChargeNumber charge(rand() % (int(mass) + 1));
    FermiFloat energy = (rand() % 1000) * CLHEP::MeV * FermiFloat(mass);
    auto vec = Randomizer::IsotropicVector() * (rand() % 1000) * CLHEP::MeV;
    auto mom = LorentzVector(vec, std::sqrt(std::pow(NucleiProperties().GetNuclearMass(mass, charge) + energy, 2) +
        vec.mag2()));
    auto particle = FermiParticle(mass, charge, mom);
    for (size_t i = 0; i < runs; ++i) {
      MassNumber mass_sum(0);
      ChargeNumber charge_sum(0);
      auto particles = model.BreakItUp(particle);
      for (auto& fragment : particles) {
        mass_sum = MassNumber(FermiUInt(mass_sum) + FermiUInt(fragment.GetMassNumber()));
        charge_sum = ChargeNumber(FermiUInt(charge_sum) + FermiUInt(fragment.GetChargeNumber()));
      }
      ASSERT_EQ(mass_sum, mass);
      ASSERT_EQ(charge_sum, charge);
    }
  }
}