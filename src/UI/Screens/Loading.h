#pragma once

#include "../Screen.h"

#pragma region ForwardDecl
class TaskCompletionView;
#pragma endregion

class LoadingScreen : public Screen {
   private:
    double startTime_;

    void draw_() override;

   public:
    TaskCompletionView *task;

    LoadingScreen() = default;
    virtual ~LoadingScreen() = default;

    void open(TaskCompletionView *task);


};