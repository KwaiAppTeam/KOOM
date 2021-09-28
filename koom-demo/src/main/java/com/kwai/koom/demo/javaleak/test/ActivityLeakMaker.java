/*
 * Copyright 2020 Kwai, Inc. All rights reserved.
 * <p>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * <p>
 * http://www.apache.org/licenses/LICENSE-2.0
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */

package com.kwai.koom.demo.javaleak.test;

import java.util.List;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

public class ActivityLeakMaker extends LeakMaker<Activity> {

  @Override
  public void startLeak(Context context) {
    LeakedActivity.setUselessObjectList(uselessObjectList);
    Intent intent = new Intent(context, LeakedActivity.class);
    context.startActivity(intent);
  }

  public static class LeakedActivity extends AppCompatActivity {

    static List<Activity> uselessObjectList;

    public static void setUselessObjectList(List<Activity> activities) {
      uselessObjectList = activities;
    }

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);
      uselessObjectList.add(this);
      this.finish();
    }
  }

}
