package com.example.gifencoder

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView
import com.example.gifencoder.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        binding.sampleText.text = decodeWebp("")
    }

    external fun stringFromJNI(): String
    external fun decodeWebp(path: String): String

    companion object {
        init {
            System.loadLibrary("gifencoder")
        }
    }
}