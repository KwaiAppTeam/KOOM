package com.kwai.koom.demo.javaleak;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.kwai.koom.demo.R;
import com.kwai.koom.demo.javaleak.test.LeakMaker;

public class JavaLeakTestActivity extends AppCompatActivity {

  public static void start(Context context) {
    context.startActivity(new Intent(context, JavaLeakTestActivity.class));
  }

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_java_leak_test);

    findViewById(R.id.btn_make_java_leak).setOnClickListener((view) -> {
      findViewById(R.id.btn_make_java_leak).setVisibility(View.INVISIBLE);
      LeakMaker.makeLeak(this);
    });
  }
}
