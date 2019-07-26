/*
 Copyright (C) AC SOFTWARE SP. Z O.O.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "CDBaseTest.h"
#include "cdbase.h"       // NOLINT
#include "gtest/gtest.h"  // NOLINT

namespace {

class CDBaseTest : public ::testing::Test {};

TEST_F(CDBaseTest, setSvrConnToNull) {
  cdbase *cd = new cdbase((serverconnection *)1);
  ASSERT_FALSE(cd == NULL);

  ASSERT_TRUE(cd->retainPtr() == cd);
  ASSERT_FALSE(cd->setSvrConnToNull());
  cd->releasePtr();
  ASSERT_TRUE(cd->setSvrConnToNull());

  delete cd;
}

}  // namespace