#pragma once

#include "../Screen.h"

#pragma region ForwardDecl
class TaskCompletionView;
#pragma endregion

class LoadingScreen : public Screen {
   private:
    double startTime_;

   public:
    TaskCompletionView &task;

    LoadingScreen(TaskCompletionView &task);
    virtual ~LoadingScreen();

    void draw() override;
};