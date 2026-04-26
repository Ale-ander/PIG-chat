package pig.puorco.pigchat

import android.content.Context;
import androidx.compose.runtime.getValue
import androidx.compose.runtime.setValue
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

import pig.puorco.pigchat.ui.theme.PigChatTheme

import android.media.MediaPlayer
import android.provider.MediaStore
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.platform.LocalContext

import java.net.HttpURLConnection
import java.net.URL
import androidx.core.content.edit

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge();

        setContent {
            PigChatTheme {
                val context = LocalContext.current

                LaunchedEffect(Unit) {
                    val mediaPlayer = MediaPlayer.create(context, R.raw.pigsound)
                    mediaPlayer.setOnCompletionListener { it.release() }
                    mediaPlayer.start()
                }

                val currentContext = LocalContext.current
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    Box(modifier = Modifier.padding(innerPadding)) {
                       MainNavigation(currentContext)
                    }
                }
            }
        }
    }
}

@Composable
fun MainNavigation(context: android.content.Context) {
    val savedIpFromStorage = remember { Storage.getIp(context) }

    var currentScreen by remember {
        mutableStateOf(if (savedIpFromStorage.equals("MAIALE")) "settings" else "chat")
    }
    var savedIp by remember { mutableStateOf(Storage.getIp(context)) }

    if (currentScreen == "chat") {
        ChatInputScreen(serverIp = savedIp,
            onGoToSettings = { currentScreen = "settings" })
    } else {
        SettingsScreen(
            currentIp = savedIp,
            onSave = { newIp ->
                Storage.saveIp(context, newIp)
                savedIp = newIp
                currentScreen = "chat"
            }
        )
    }
}

@Composable
fun ChatInputScreen(serverIp: String, onGoToSettings: () -> Unit) {
    var textInput by remember { mutableStateOf("") }
    var displayedText by remember { mutableStateOf("\uD83D\uDC37") }
    val scope = rememberCoroutineScope()

    fun sendMessage() {
        if (textInput.isNotBlank()) {
            val messageToSend = textInput
            textInput = ""
            displayedText = "Oinking in Progress..."

            scope.launch {
                performHttpRequest(serverIp, messageToSend)
                displayedText = "\uD83D\uDC37"
            }
        }
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Top
    ) {
        Row(modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.End) {
            TextButton(onClick = onGoToSettings) { Text("Settings Porcile") }
        }

       Text(text = displayedText, style = MaterialTheme.typography.headlineMedium)

       Spacer(modifier = Modifier.height(20.dp))

        OutlinedTextField(
            value = textInput,
            onValueChange = { textInput = it },
            label = { "Message" },
            modifier = Modifier.fillMaxWidth(),
            singleLine = true,
            keyboardOptions = KeyboardOptions( imeAction = ImeAction.Send ),
            keyboardActions = KeyboardActions( onSend = { sendMessage() }
            )
        )

        Spacer(modifier = Modifier.height(16.dp))

        Button(
            onClick = { sendMessage() },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Oink")
        }
    }
}

@Composable
fun SettingsScreen(currentIp: String, onSave: (String) -> Unit) {
    var ipInput by remember { mutableStateOf(currentIp) }

    Column(
        modifier = Modifier.fillMaxSize().padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Top
    ) {
        Text(text = "Server Setup", style = MaterialTheme.typography.titleLarge)
        Spacer(modifier = Modifier.height(16.dp))

        OutlinedTextField(
            value = ipInput,
            onValueChange = { ipInput = it },
            label = { Text("IP Puorcu") },
            modifier = Modifier.fillMaxWidth()
        )
        Spacer(modifier = Modifier.height(16.dp))
        Button(
            onClick = { onSave(ipInput) },
            modifier = Modifier.fillMaxWidth()) {
           Text("Save IP Puorcu")
        }
    }
}

suspend fun performHttpRequest(ip: String, data: String): String {
    return withContext(Dispatchers.IO) {
        var url = URL("http://$ip/recv")
        var client: HttpURLConnection? = null

        try {
            client = url.openConnection() as HttpURLConnection
            client.requestMethod = "POST"
            client.doOutput = true
            client.connectTimeout = 10000

            client.outputStream.use { os ->
                os.write(data.toByteArray(Charsets.UTF_8))
                os.flush()
                os.close()
            }
            val responseCode = client.responseCode
            if (responseCode == HttpURLConnection.HTTP_OK) {
                ""
            } else {
                "Error: ${responseCode}"
            }
        } catch (e: Exception) {
            "Error: ${e.localizedMessage}"
        } finally {
            client?.disconnect()
        }
    }
}

object Storage {
    private const val PREFS_NAME = "PigChatPrefs"
    private const val KEY_IP = "puorcu_ip"

    fun saveIp(context: Context, ip: String) {
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        prefs.edit { putString(KEY_IP, ip) }
    }

    fun getIp(context: Context): String {
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        return prefs.getString(KEY_IP, "MAIALE") ?: "MAIALE"
    }
}