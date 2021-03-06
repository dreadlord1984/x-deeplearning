/* Copyright (C) 2016-2018 Alibaba Group Holding Limited

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "xdl/data_io/scheduler.h"
#include "gtest/gtest.h"

#include <stdlib.h>

namespace xdl {
namespace io {

static const char *path = "sample.txt";
static const char *path2 = "sample2.txt";

TEST(DataIOTest, TestSchedule) {
  Scheduler sched(kLocal);
  sched.SetEpochs(2);

  sched.AddPath(path);
  sched.AddPath(path2);

  ASSERT_TRUE(sched.Schedule());

  ReadParam *rparam = nullptr;

  for (int i = 0; i < 6; ++i) {
    rparam = sched.Acquire();
    if (i >= 4) {
      ASSERT_EQ(nullptr, rparam);
      continue;
    }
    ASSERT_NE(nullptr, rparam);
    EXPECT_EQ(0, rparam->begin_);
    EXPECT_EQ(i<2?0:1, rparam->epoch_);
    EXPECT_GT(rparam->end_, 0);
    EXPECT_NE(nullptr, rparam->ant_);
  }
}


TEST(DataIOTest, TestRestore) {
  Scheduler sched(kLocal);
  sched.SetEpochs(2);

  sched.AddPath(path);
  sched.AddPath(path2);

  sched.Schedule();

  auto rparam = sched.Acquire();
  ASSERT_NE(nullptr, rparam);
  EXPECT_EQ(0, rparam->begin_);
  EXPECT_EQ(0, rparam->epoch_);
  EXPECT_GT(rparam->end_, 0);
  EXPECT_STREQ(rparam->path_, path);
  EXPECT_NE(nullptr, rparam->ant_);

  rparam->begin_ = 1;

  rparam = sched.Acquire();
  ASSERT_NE(nullptr, rparam);
  EXPECT_EQ(0, rparam->begin_);
  EXPECT_EQ(0, rparam->epoch_);
  EXPECT_GT(rparam->end_, 0);
  EXPECT_STREQ(rparam->path_, path2);
  EXPECT_NE(nullptr, rparam->ant_);

  rparam->begin_ = 1;

  DSState ds_state;
  sched.Store(&ds_state);

  EXPECT_EQ(4, ds_state.states_size());
  EXPECT_EQ(2, ds_state.epochs());
  for (int i = 0; i < ds_state.states_size(); ++i) {
    auto state = ds_state.states(i);
    EXPECT_EQ(i<2?1:0, state.begin());
    EXPECT_EQ(i<2?0:1, state.epoch());
    EXPECT_GT(state.end(), 0);
    //EXPECT_STREQ(state.path().c_str(), i%2==0?path:path2);
  }

  std::cout << ds_state.ShortDebugString() << std::endl;

  Scheduler sched2(kLocal);
  sched2.Restore(ds_state);

  ASSERT_FALSE(sched2.Schedule());

  for (int i = 0; i < 6; ++i) {
    rparam = sched2.Acquire();
    if (i >= 4) {
      ASSERT_EQ(nullptr, rparam);
      continue;
    }
    ASSERT_NE(nullptr, rparam);
    EXPECT_EQ(i<2?1:0, rparam->begin_);
    EXPECT_EQ(i<2?0:1, rparam->epoch_);
    EXPECT_GT(rparam->end_, 0);
    EXPECT_NE(nullptr, rparam->ant_);
  }
}

}
}
