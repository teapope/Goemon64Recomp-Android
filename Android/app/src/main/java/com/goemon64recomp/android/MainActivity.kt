package com.goemon64recomp.android

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import android.util.Log
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.view.WindowManager
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.lifecycle.lifecycleScope
import com.goemon64recomp.android.databinding.ActivityMainBinding
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.async
import kotlinx.coroutines.launch
import java.io.File

class MainActivity : AppCompatActivity(), SurfaceHolder.Callback {
    
    private lateinit var binding: ActivityMainBinding
    private var surfaceView: SurfaceView? = null
    private var isGameInitialized = false
    private var isGameRunning = false
    private var romPath: String? = null
    
    companion object {
        private const val TAG = "Goemon64MainActivity"
        private const val STORAGE_PERMISSION_REQUEST = 100
        private const val ROM_PICKER_REQUEST = 101
        
        // Native methods
        init {
            System.loadLibrary("goemon64-recomp")
        }
    }
    
    // Native function declarations
    private external fun initNativeEngine(surface: Surface, dataPath: String): Boolean
    private external fun loadROM(romPath: String): Boolean
    private external fun startGame(): Boolean
    private external fun stopGame()
    private external fun pauseGame()
    private external fun resumeGame()
    private external fun onSurfaceChanged(width: Int, height: Int)
    private external fun processInput(action: Int, keyCode: Int, x: Float, y: Float): Boolean
    private external fun renderFrame()
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        
        setupFullscreen()
        setupSurface()
        
        // Check for permissions
        if (checkStoragePermissions()) {
            initializeGame()
        } else {
            requestStoragePermissions()
        }
    }
    
    private fun setupFullscreen() {
        // Keep screen on
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        
        // Hide system UI
        WindowCompat.setDecorFitsSystemWindows(window, false)
        val controller = WindowInsetsControllerCompat(window, binding.root)
        controller.hide(WindowInsetsCompat.Type.systemBars())
        controller.systemBarsBehavior = 
            WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
    }
    
    private fun setupSurface() {
        surfaceView = binding.gameSurface
        surfaceView?.holder?.addCallback(this)
    }
    
    private fun checkStoragePermissions(): Boolean {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            Environment.isExternalStorageManager()
        } else {
            ContextCompat.checkSelfPermission(
                this,
                Manifest.permission.READ_EXTERNAL_STORAGE
            ) == PackageManager.PERMISSION_GRANTED &&
            ContextCompat.checkSelfPermission(
                this,
                Manifest.permission.WRITE_EXTERNAL_STORAGE
            ) == PackageManager.PERMISSION_GRANTED
        }
    }
    
    private fun requestStoragePermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            try {
                val intent = Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION)
                intent.data = Uri.parse("package:$packageName")
                startActivityForResult(intent, STORAGE_PERMISSION_REQUEST)
            } catch (e: Exception) {
                val intent = Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
                startActivityForResult(intent, STORAGE_PERMISSION_REQUEST)
            }
        } else {
            ActivityCompat.requestPermissions(
                this,
                arrayOf(
                    Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE
                ),
                STORAGE_PERMISSION_REQUEST
            )
        }
    }
    
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        
        if (requestCode == STORAGE_PERMISSION_REQUEST) {
            if (grantResults.all { it == PackageManager.PERMISSION_GRANTED }) {
                initializeGame()
            } else {
                showPermissionDeniedDialog()
            }
        }
    }
    
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        
        when (requestCode) {
            STORAGE_PERMISSION_REQUEST -> {
                if (checkStoragePermissions()) {
                    initializeGame()
                } else {
                    showPermissionDeniedDialog()
                }
            }
            ROM_PICKER_REQUEST -> {
                if (resultCode == Activity.RESULT_OK) {
                    data?.data?.let { uri ->
                        handleRomSelection(uri)
                    }
                }
            }
        }
    }
    
    private fun showPermissionDeniedDialog() {
        AlertDialog.Builder(this)
            .setTitle("Permissions Required")
            .setMessage("Storage permissions are required to load game data. Please grant permissions in settings.")
            .setPositiveButton("Settings") { _, _ ->
                val intent = Intent(Settings.APPLICATION_DETAILS_SETTINGS)
                intent.data = Uri.parse("package:$packageName")
                startActivity(intent)
            }
            .setNegativeButton("Exit") { _, _ ->
                finish()
            }
            .setCancelable(false)
            .show()
    }
    
    private fun initializeGame() {
        lifecycleScope.launch {
            try {
                // Initialize data directories
                val dataPath = getExternalFilesDir(null)?.absolutePath 
                    ?: throw Exception("Could not access app data directory")
                
                // Check if ROM is already available
                val romFile = File(dataPath, "goemon.z64")
                if (romFile.exists()) {
                    romPath = romFile.absolutePath
                    showRomConfirmDialog(romFile.absolutePath)
                } else {
                    showRomPickerDialog()
                }
            } catch (e: Exception) {
                Log.e(TAG, "Failed to initialize game", e)
                showError("Initialization failed: ${e.message}")
            }
        }
    }
    
    private fun showRomPickerDialog() {
        AlertDialog.Builder(this)
            .setTitle("Select ROM")
            .setMessage("No ROM found. Would you like to select your Mystical Ninja Starring Goemon ROM?")
            .setPositiveButton("Select ROM") { _, _ ->
                openRomPicker()
            }
            .setNegativeButton("Exit") { _, _ ->
                finish()
            }
            .setCancelable(false)
            .show()
    }
    
    private fun showRomConfirmDialog(path: String) {
        AlertDialog.Builder(this)
            .setTitle("ROM Found")
            .setMessage("Found existing ROM. Would you like to use it?\n\nPath: $path")
            .setPositiveButton("Yes") { _, _ ->
                loadAndStartGame(path)
            }
            .setNegativeButton("Select Different ROM") { _, _ ->
                openRomPicker()
            }
            .show()
    }
    
    private fun openRomPicker() {
        val intent = Intent(Intent.ACTION_GET_CONTENT).apply {
            type = "*/*"
            addCategory(Intent.CATEGORY_OPENABLE)
        }
        startActivityForResult(intent, ROM_PICKER_REQUEST)
    }
    
    private fun handleRomSelection(uri: Uri) {
        lifecycleScope.launch {
            try {
                binding.loadingProgress.visibility = View.VISIBLE
                binding.loadingText.text = "Copying ROM..."
                
                // Copy ROM to app data directory
                val dataPath = getExternalFilesDir(null)?.absolutePath 
                    ?: throw Exception("Could not access app data directory")
                val destFile = File(dataPath, "goemon.z64")
                
                contentResolver.openInputStream(uri)?.use { input ->
                    destFile.outputStream().use { output ->
                        input.copyTo(output)
                    }
                }
                
                romPath = destFile.absolutePath
                loadAndStartGame(destFile.absolutePath)
                
            } catch (e: Exception) {
                Log.e(TAG, "Failed to copy ROM", e)
                binding.loadingProgress.visibility = View.GONE
                showError("Failed to load ROM: ${e.message}")
            }
        }
    }
    
    private fun loadAndStartGame(romPath: String) {
        lifecycleScope.launch {
            try {
                binding.loadingProgress.visibility = View.VISIBLE
                binding.loadingText.text = "Loading game..."
                
                val dataPath = getExternalFilesDir(null)?.absolutePath 
                    ?: throw Exception("Could not access app data directory")
                
                // Initialize native engine
                surfaceView?.holder?.surface?.let { surface ->
                    if (!initNativeEngine(surface, dataPath)) {
                        throw Exception("Failed to initialize native engine")
                    }
                }
                
                // Load ROM
                if (!loadROM(romPath)) {
                    throw Exception("Failed to load ROM. Please ensure you have the correct US version.")
                }
                
                // Start game
                if (!startGame()) {
                    throw Exception("Failed to start game")
                }
                
                isGameInitialized = true
                isGameRunning = true
                
                binding.loadingProgress.visibility = View.GONE
                binding.loadingText.visibility = View.GONE
                
                // Start render loop
                startRenderLoop()
                
            } catch (e: Exception) {
                Log.e(TAG, "Failed to start game", e)
                binding.loadingProgress.visibility = View.GONE
                showError("Failed to start game: ${e.message}")
            }
        }
    }
    
    private fun startRenderLoop() {
        lifecycleScope.launch {
            while (isGameRunning) {
                try {
                    renderFrame()
                } catch (e: Exception) {
                    Log.e(TAG, "Render error", e)
                    break
                }
            }
        }
    }
    
    private fun showError(message: String) {
        runOnUiThread {
            Toast.makeText(this, message, Toast.LENGTH_LONG).show()
        }
    }
    
    // SurfaceHolder.Callback implementation
    override fun surfaceCreated(holder: SurfaceHolder) {
        Log.d(TAG, "Surface created")
    }
    
    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        Log.d(TAG, "Surface changed: ${width}x${height}")
        if (isGameInitialized) {
            onSurfaceChanged(width, height)
        }
    }
    
    override fun surfaceDestroyed(holder: SurfaceHolder) {
        Log.d(TAG, "Surface destroyed")
        isGameRunning = false
    }
    
    // Input handling
    override fun onKeyDown(keyCode: Int, event: KeyEvent?): Boolean {
        if (isGameRunning) {
            return processInput(KeyEvent.ACTION_DOWN, keyCode, 0f, 0f)
        }
        return super.onKeyDown(keyCode, event)
    }
    
    override fun onKeyUp(keyCode: Int, event: KeyEvent?): Boolean {
        if (isGameRunning) {
            // Handle back button for menu
            if (keyCode == KeyEvent.KEYCODE_BACK) {
                // Toggle enhancement menu
                // This will be handled by native code
            }
            return processInput(KeyEvent.ACTION_UP, keyCode, 0f, 0f)
        }
        return super.onKeyUp(keyCode, event)
    }
    
    override fun onTouchEvent(event: MotionEvent?): Boolean {
        event?.let {
            if (isGameRunning) {
                return processInput(it.action, -1, it.x, it.y)
            }
        }
        return super.onTouchEvent(event)
    }
    
    // Lifecycle management
    override fun onPause() {
        super.onPause()
        if (isGameRunning) {
            pauseGame()
        }
    }
    
    override fun onResume() {
        super.onResume()
        if (isGameInitialized && isGameRunning) {
            resumeGame()
        }
    }
    
    override fun onDestroy() {
        isGameRunning = false
        if (isGameInitialized) {
            stopGame()
        }
        super.onDestroy()
    }
}
