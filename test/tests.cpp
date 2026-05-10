// Copyright 2021 GHA Test Team
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

#include "TimedDoor.h"

using ::testing::InSequence;
using ::testing::Return;

class MockTimerClient : public TimerClient {
 public:
  MOCK_METHOD(void, Timeout, (), (override));
};

class MockDoor : public Door {
 public:
  MOCK_METHOD(void, lock, (), (override));
  MOCK_METHOD(void, unlock, (), (override));
  MOCK_METHOD(bool, isDoorOpened, (), (override));
};

class DoorUser {
 public:
  bool EnsureClosed(Door& door) {
    if (door.isDoorOpened()) {
      door.lock();
      return true;
    }
    return false;
  }

  void OpenThenClose(Door& door) {
    door.unlock();
    door.lock();
  }
};

class TimedDoorFixture : public ::testing::Test {
 protected:
  TimedDoor* timedDoor;

  void SetUp() override {
    timedDoor = new TimedDoor(0);
  }

  void TearDown() override {
    delete timedDoor;
    timedDoor = nullptr;
  }
};

TEST_F(TimedDoorFixture, ConstructorCreatesClosedDoor) {
  EXPECT_FALSE(timedDoor->isDoorOpened());
}

TEST_F(TimedDoorFixture, LockKeepsDoorClosed) {
  timedDoor->lock();
  EXPECT_FALSE(timedDoor->isDoorOpened());
}

TEST_F(TimedDoorFixture, ThrowStateDoesNotThrowWhenClosed) {
  timedDoor->lock();
  EXPECT_NO_THROW(timedDoor->throwState());
}

TEST_F(TimedDoorFixture, UnlockThrowsWhenDoorStaysOpened) {
  EXPECT_THROW(timedDoor->unlock(), std::runtime_error);
}

TEST(TimedDoorStateTest, TimeoutGetterReturnsConstructorValue) {
  TimedDoor door(42);
  EXPECT_EQ(door.getTimeOut(), 42);
}

TEST(TimedDoorStateTest, AdapterTimeoutThrowsWhenDoorOpened) {
  TimedDoor door(0);
  EXPECT_THROW(door.unlock(), std::runtime_error);
  DoorTimerAdapter adapter(door);
  EXPECT_THROW(adapter.Timeout(), std::runtime_error);
}

TEST(TimedDoorStateTest, AdapterTimeoutDoesNotThrowWhenDoorClosed) {
  TimedDoor door(0);
  door.lock();
  DoorTimerAdapter adapter(door);
  EXPECT_NO_THROW(adapter.Timeout());
}

TEST(TimerTest, RegisterCallsClientTimeoutOnce) {
  Timer timer;
  MockTimerClient client;
  EXPECT_CALL(client, Timeout()).Times(1);
  timer.tregister(0, &client);
}

TEST(TimerTest, RegisterWithNullClientDoesNotThrow) {
  Timer timer;
  EXPECT_NO_THROW(timer.tregister(0, nullptr));
}

TEST(DoorInterfaceTest, EnsureClosedCallsLockWhenOpened) {
  MockDoor door;
  DoorUser user;
  EXPECT_CALL(door, isDoorOpened()).WillOnce(Return(true));
  EXPECT_CALL(door, lock()).Times(1);
  EXPECT_TRUE(user.EnsureClosed(door));
}

TEST(DoorInterfaceTest, EnsureClosedDoesNotCallLockWhenAlreadyClosed) {
  MockDoor door;
  DoorUser user;
  EXPECT_CALL(door, isDoorOpened()).WillOnce(Return(false));
  EXPECT_CALL(door, lock()).Times(0);
  EXPECT_FALSE(user.EnsureClosed(door));
}

TEST(DoorInterfaceTest, OpenThenCloseCallsMethodsInOrder) {
  MockDoor door;
  DoorUser user;
  InSequence sequence;
  EXPECT_CALL(door, unlock()).Times(1);
  EXPECT_CALL(door, lock()).Times(1);
  user.OpenThenClose(door);
}
