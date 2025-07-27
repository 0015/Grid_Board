import 'dart:io';
import 'dart:async';
import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:shared_preferences/shared_preferences.dart';

const String targetDeviceName = "Grid_Board";
const String serviceUUID = "00ff";
const String charUUID = "ff01";

class SavedGrid {
  final String id;
  final String name;
  final List<List<String>> grid;
  final DateTime savedAt;

  SavedGrid({
    required this.id,
    required this.name,
    required this.grid,
    required this.savedAt,
  });

  Map<String, dynamic> toJson() {
    return {
      'id': id,
      'name': name,
      'grid': grid,
      'savedAt': savedAt.toIso8601String(),
    };
  }

  factory SavedGrid.fromJson(Map<String, dynamic> json) {
    return SavedGrid(
      id: json['id'],
      name: json['name'],
      grid: List<List<String>>.from(
          json['grid'].map((row) => List<String>.from(row))
      ),
      savedAt: DateTime.parse(json['savedAt']),
    );
  }
}

void main() {
  runApp(const MaterialApp(debugShowCheckedModeBanner: false, home: LoadingScreen()));
}


class LoadingScreen extends StatefulWidget {
  const LoadingScreen({super.key});

  @override
  State<LoadingScreen> createState() => _LoadingScreenState();
}

class _LoadingScreenState extends State<LoadingScreen> {
  String status = "Scanning for Grid Board...";
  bool scanFailed = false;

  @override
  void initState() {
    super.initState();
    scanAndConnect();
  }

  Future<void> scanAndConnect() async {
    BluetoothDevice? targetDevice;
    BluetoothCharacteristic? targetChar;

    try {
      // Wait for adapter to be powered on
      await FlutterBluePlus.adapterState
          .firstWhere((state) => state == BluetoothAdapterState.on);

      if (Platform.isAndroid) {
        await FlutterBluePlus.startScan(timeout: const Duration(seconds: 10), androidUsesFineLocation: true);
      } else if (Platform.isIOS) {
        await FlutterBluePlus.startScan(timeout: const Duration(seconds: 10));
      }else{
        return;
      }

      FlutterBluePlus.scanResults.listen((results) async {
        for (ScanResult r in results) {
          if (r.device.advName == targetDeviceName) {
            targetDevice = r.device;
            await FlutterBluePlus.stopScan();
            await targetDevice!.connect();
            List<BluetoothService> services = await targetDevice!.discoverServices();
            for (var service in services) {
              if (service.uuid.toString().contains(serviceUUID)) {
                for (var c in service.characteristics) {
                  if (c.uuid.toString().contains(charUUID)) {
                    targetChar = c;
                    Navigator.pushReplacement(
                      context,
                      MaterialPageRoute(
                        builder: (_) => GridScreen(characteristic: targetChar!),
                      ),
                    );
                    return;
                  }
                }
              }
            }
          }
        }

        // Timeout fallback
        Future.delayed(const Duration(seconds: 10), () {
          if (targetChar == null && mounted) {
            setState(() {
              scanFailed = true;
              status = "Not Found";
            });
          }
        });
      });
    } catch (e) {
      setState(() {
        scanFailed = true;
        status = "Error: $e";
      });
    }
  }


  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Center(
        child: scanFailed
            ? Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(status, style: const TextStyle(fontSize: 20)),
            const SizedBox(height: 20),
            ElevatedButton(
              onPressed: () {
                setState(() {
                  status = "Retrying...";
                  scanFailed = false;
                });
                scanAndConnect();
              },
              child: const Text("Retry"),
            ),
          ],
        )
            : Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const CircularProgressIndicator(),
            const SizedBox(height: 20),
            Text(status, style: const TextStyle(fontSize: 20)),
          ],
        ),
      ),
    );
  }
}



class GridScreen extends StatefulWidget {
  final BluetoothCharacteristic characteristic;

  const GridScreen({super.key, required this.characteristic});

  @override
  State<GridScreen> createState() => _GridScreenState();
}

class _GridScreenState extends State<GridScreen> {
  final int rows = 5;
  final int cols = 12;
  final List<List<String>> slotGrid =
  List.generate(5, (_) => List.filled(12, " "));

  int cursorRow = 2;
  int cursorCol = 0;

  final FocusNode _textFocus = FocusNode();
  final TextEditingController _textController = TextEditingController();
  String _previousText = '';

  static const List<String> emojiList = [
    "✅", "✔", "✖", "❌", "❤", "📀", "📁", "📂", "📃", "📄", "📅", "📆", "📇", "📈", "📉", "📊", "📋", "📌", "📍", "📎", "📏", "📐", "📑", "📒", "📓", "📔", "📕", "📖", "📗", "📘", "📙", "📚", "📛", "📜", "📝", "📞", "📟", "📠", "📡", "📢", "📣", "📤", "📥", "📦", "📧", "📨", "📩", "📪", "📫", "📬", "📭", "📮", "📯", "📰", "📱", "📲", "📳", "📴", "📵", "📶", "📷", "📸", "📹", "📺", "📻", "📼", "📽", "📿", "😀", "😁", "😂", "😃", "😄", "😅", "😆", "😇", "😈", "😉", "😊", "😋", "😌", "😍", "😎", "😏", "😐", "😑", "😒", "😓", "😔", "😕", "😖", "😗", "😘", "😙", "😚", "😛", "😜", "😝", "😞", "😟", "😠", "😡", "😢", "😣", "😤", "😥", "😦", "😧", "😨", "😩", "😪", "😫", "😬", "😭", "😮", "😯", "😰", "😱", "😲", "😳", "😴", "😵", "😶", "😷", "😸", "😹", "😺", "😻", "😼", "😽", "😾", "😿", "🙀", "🙁", "🙂", "🙃", "🙄", "🙅", "🙆", "🙇", "🙈", "🙉", "🙊", "🙋", "🙌", "🙍", "🙎", "🙏", "🚀", "🚁", "🚂", "🚃", "🚄", "🚅", "🚆", "🚇", "🚈", "🚉", "🚊", "🚋", "🚌", "🚍", "🚎", "🚏", "🚐", "🚑", "🚒", "🚓", "🚔", "🚕", "🚖", "🚗", "🚘", "🚙", "🚚", "🚛", "🚜", "🚝", "🚞", "🚟", "🚠", "🚡", "🚢", "🚣", "🚤", "🚥", "🚦", "🚧", "🚨", "🚩", "🚪", "🚫", "🚬", "🚭", "🚮", "🚯", "🚰", "🚱", "🚲", "🚳", "🚴", "🚵", "🚶", "🚷", "🚸", "🚹", "🚺", "🚻", "🚼", "🚽", "🚾", "🚿", "🛀", "🛁", "🛂", "🛃", "🛄", "🛅", "🛋", "🛌", "🛍", "🛎", "🛏", "🛐", "🛑", "🛒", "🛕", "🛖", "🛗", "🛜", "🛝", "🛞", "🛟", "🛠", "🛡", "🛢", "🛣", "🛤", "🛥", "🛩", "🛫", "🛬", "🛰", "🛳", "🛴", "🛵", "🛶", "🛷", "🛸", "🛹", "🛺", "🛻", "🛼"
  ];

  void _handleTextChange(String text) {
    print("Text changed: '$text', previous: '$_previousText'");

    if (text.length < _previousText.length) {
      // Backspace detected - text got shorter
      setState(() {
        slotGrid[cursorRow][cursorCol] = " ";
        moveCursorBack();
      });
    } else if (text.length > _previousText.length && text.isNotEmpty) {
      // New character added
      final char = text.characters.last.toUpperCase();
      setState(() {
        slotGrid[cursorRow][cursorCol] = char;
        moveCursorForward();
      });
    }

    // Update previous text and clear controller
    _previousText = text;
    Future.microtask(() {
      _textController.clear();
      _previousText = '';
    });
  }

  @override
  void initState() {
    super.initState();

    WidgetsBinding.instance.addPostFrameCallback((_) {
      FocusScope.of(context).requestFocus(_textFocus);
    });
  }

  void moveCursorForward() {
    cursorCol++;
    if (cursorCol >= cols) {
      cursorCol = 0;
      cursorRow++;
      if (cursorRow >= rows) {
        cursorRow = 0;
      }
    }
  }

  void moveCursorBack() {
    if (cursorCol > 0) {
      cursorCol--;
    } else if (cursorRow > 0) {
      cursorRow--;
      cursorCol = cols - 1;
    }
  }

  void onCellTapped(int row, int col) {
    setState(() {
      cursorRow = row;
      cursorCol = col;
    });

    Future.delayed(const Duration(milliseconds: 100), () {
      FocusScope.of(context).requestFocus(_textFocus);
    });
  }

  void onCellLongPressed(int row, int col) async {
    String? selectedEmoji = await showDialog<String>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text("Select Emoji"),
        content: SizedBox(
          width: double.maxFinite,
          height: 300,
          child: GridView.count(
            crossAxisCount: 6,
            children: emojiList.map((emoji) {
              return GestureDetector(
                onTap: () => Navigator.pop(context, emoji),
                child: Center(child: Text(emoji, style: const TextStyle(fontSize: 24))),
              );
            }).toList(),
          ),
        ),
      ),
    );

    if (selectedEmoji != null) {
      setState(() {
        cursorRow = row;
        cursorCol = col;
        slotGrid[row][col] = selectedEmoji;
        moveCursorForward();
      });
    }
  }

  void clearGrid() {
    setState(() {
      for (var row = 0; row < rows; row++) {
        for (var col = 0; col < cols; col++) {
          slotGrid[row][col] = ' ';
        }
      }
      cursorRow = 2;
      cursorCol = 0;
    });
  }

  Future<void> saveGrid() async {
    String? gridName = await showDialog<String>(
      context: context,
      builder: (context) {
        String name = '';
        return AlertDialog(
          title: const Text('Save Grid'),
          content: TextField(
            onChanged: (value) => name = value,
            decoration: const InputDecoration(
              hintText: 'Enter grid name...',
              border: OutlineInputBorder(),
            ),
            autofocus: true,
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context),
              child: const Text('Cancel'),
            ),
            TextButton(
              onPressed: () => Navigator.pop(context, name.trim()),
              child: const Text('Save'),
            ),
          ],
        );
      },
    );

    if (gridName != null && gridName.isNotEmpty) {
      final prefs = await SharedPreferences.getInstance();
      final savedGrids = await _loadSavedGrids();

      final newGrid = SavedGrid(
        id: DateTime.now().millisecondsSinceEpoch.toString(),
        name: gridName,
        grid: slotGrid.map((row) => List<String>.from(row)).toList(),
        savedAt: DateTime.now(),
      );

      savedGrids.add(newGrid);

      final gridsJson = savedGrids.map((grid) => grid.toJson()).toList();
      await prefs.setString('saved_grids', jsonEncode(gridsJson));

      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Grid "$gridName" saved successfully!')),
        );
      }
    }
  }

  Future<List<SavedGrid>> _loadSavedGrids() async {
    final prefs = await SharedPreferences.getInstance();
    final gridsJson = prefs.getString('saved_grids');

    if (gridsJson == null) return [];

    final List<dynamic> gridsList = jsonDecode(gridsJson);
    return gridsList.map((json) => SavedGrid.fromJson(json)).toList();
  }

  void _showLoadDialog() async {
    final savedGrids = await _loadSavedGrids();

    if (savedGrids.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('No saved grids found!')),
      );
      return;
    }

    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Load Grid'),
        content: SizedBox(
          width: double.maxFinite,
          height: (savedGrids.length * 220.0).clamp(200.0, MediaQuery.of(context).size.height * 0.7),
          child: ListView.builder(
            itemCount: savedGrids.length,
            itemBuilder: (context, index) {
              final grid = savedGrids[index];
              return Card(
                margin: const EdgeInsets.symmetric(vertical: 4),
                child: Padding(
                  padding: const EdgeInsets.all(8),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          Expanded(
                            child: Text(
                              grid.name,
                              style: const TextStyle(
                                fontSize: 16,
                                fontWeight: FontWeight.bold,
                              ),
                            ),
                          ),
                          IconButton(
                            icon: const Icon(Icons.delete, color: Colors.red),
                            onPressed: () async {
                              savedGrids.removeAt(index);
                              final prefs = await SharedPreferences.getInstance();
                              final gridsJson = savedGrids.map((g) => g.toJson()).toList();
                              await prefs.setString('saved_grids', jsonEncode(gridsJson));
                              Navigator.pop(context);
                              _showLoadDialog(); // Refresh the dialog
                            },
                          ),
                        ],
                      ),
                      Text(
                        'Saved: ${grid.savedAt.day}/${grid.savedAt.month}/${grid.savedAt.year} '
                            '${grid.savedAt.hour.toString().padLeft(2, '0')}:${grid.savedAt.minute.toString().padLeft(2, '0')}',
                        style: TextStyle(
                          fontSize: 12,
                          color: Colors.grey[600],
                        ),
                      ),
                      const SizedBox(height: 8),
                      // Preview Grid
                      Container(
                        height: 120,
                        decoration: BoxDecoration(
                          border: Border.all(color: Colors.grey[300]!),
                          borderRadius: BorderRadius.circular(4),
                        ),
                        child: GridView.builder(
                          physics: const NeverScrollableScrollPhysics(),
                          itemCount: 60, // 5 rows × 12 cols
                          gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                            crossAxisCount: 12,
                            mainAxisSpacing: 1,
                            crossAxisSpacing: 1,
                          ),
                          itemBuilder: (context, cellIndex) {
                            int row = cellIndex ~/ 12;
                            int col = cellIndex % 12;
                            String cellContent = ' ';

                            if (row < grid.grid.length && col < grid.grid[row].length) {
                              cellContent = grid.grid[row][col];
                            }

                            return Container(
                              alignment: Alignment.center,
                              decoration: BoxDecoration(
                                color: cellContent == ' ' ? Colors.grey[100] : Colors.blue[50],
                                border: Border.all(color: Colors.grey[200]!, width: 0.5),
                              ),
                              child: Text(
                                cellContent,
                                style: const TextStyle(
                                  fontFamily: 'NotoEmoji',
                                  fontSize: 10,
                                ),
                              ),
                            );
                          },
                        ),
                      ),
                      const SizedBox(height: 8),
                      SizedBox(
                        width: double.infinity,
                        child: ElevatedButton(
                          onPressed: () {
                            setState(() {
                              for (int r = 0; r < rows; r++) {
                                for (int c = 0; c < cols; c++) {
                                  if (r < grid.grid.length && c < grid.grid[r].length) {
                                    slotGrid[r][c] = grid.grid[r][c];
                                  }
                                }
                              }
                              cursorRow = 2;
                              cursorCol = 0;
                            });
                            Navigator.pop(context);
                            ScaffoldMessenger.of(context).showSnackBar(
                              SnackBar(content: Text('Grid "${grid.name}" loaded!')),
                            );
                          },
                          child: const Text('Load This Grid'),
                        ),
                      ),
                    ],
                  ),
                ),
              );
            },
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
        ],
      ),
    );
  }

  Future<void> sendGridToESP() async {
    try {
      final device = widget.characteristic.device;
      final charUUID = widget.characteristic.uuid;
      final serviceUUID = widget.characteristic.serviceUuid;

      // If disconnected, reconnect and rediscover services/characteristics
      if (device.isDisconnected) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text("Reconnecting to Grid Board...")),
        );

        await device.connect(autoConnect: false);
        await Future.delayed(const Duration(milliseconds: 800)); // Wait a moment

        await device.discoverServices();
      }

      // Always get the latest characteristic after connect/discover
      BluetoothCharacteristic? targetChar;
      List<BluetoothService> services = await device.discoverServices();
      for (var service in services) {
        if (service.uuid == serviceUUID) {
          for (var char in service.characteristics) {
            if (char.uuid == charUUID) {
              targetChar = char;
              break;
            }
          }
        }
        if (targetChar != null) break;
      }

      if (targetChar == null) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text("Failed to find BLE characteristic.")),
        );
        return;
      }

      // Send the data
      String flatGrid = slotGrid.expand((row) => row).map((c) => c.isEmpty ? ' ' : c).join();
      if (flatGrid.length < 60) flatGrid = flatGrid.padRight(60);
      await targetChar.write(utf8.encode(flatGrid));
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text("Grid sent to ESP32")),
      );
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text("Failed to send grid: $e")),
      );
    }
  }


  @override
  void dispose() {
    _textController.dispose();
    _textFocus.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text("Card Grid"),
        actions: [
          IconButton(
            icon: const Icon(Icons.save),
            onPressed: saveGrid,
            tooltip: 'Save Grid',
          ),
          IconButton(
            icon: const Icon(Icons.folder_open),
            onPressed: _showLoadDialog,
            tooltip: 'Load Grid',
          ),
        ],
      ),
      resizeToAvoidBottomInset: false,
      body: SafeArea(
        child: KeyboardListener(
          focusNode: FocusNode(),
          autofocus: false,
          onKeyEvent: (KeyEvent event) {
            if (event is KeyDownEvent &&
                event.logicalKey == LogicalKeyboardKey.backspace) {
              setState(() {
                slotGrid[cursorRow][cursorCol] = " ";
                moveCursorBack();
              });
            }
          },
          child: Column(
            children: [
              Expanded(
                child: GridView.builder(
                  itemCount: rows * cols,
                  padding: const EdgeInsets.all(8),
                  gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                    crossAxisCount: 12,
                    mainAxisSpacing: 4,
                    crossAxisSpacing: 4,
                  ),
                  itemBuilder: (context, index) {
                    int row = index ~/ cols;
                    int col = index % cols;
                    bool isSelected = (row == cursorRow && col == cursorCol);
                    return GestureDetector(
                      onTap: () => onCellTapped(row, col),
                      onLongPress: () => onCellLongPressed(row, col),
                      child: Container(
                        alignment: Alignment.center,
                        decoration: BoxDecoration(
                          color: isSelected ? Colors.yellow[300] : Colors.grey[200],
                          border: Border.all(color: Colors.black26),
                        ),
                        child: Text(
                          slotGrid[row][col],
                          style: const TextStyle(
                            fontFamily: 'NotoEmoji',
                            fontSize: 20,
                            fontWeight: FontWeight.bold,
                          ),
                        ),
                      ),
                    );
                  },
                ),
              ),

              // Hidden TextField to trigger keyboard and detect input + backspace
              SizedBox(
                height: 0,
                width: 0,
                child: TextField(
                  focusNode: _textFocus,
                  controller: _textController,
                  onChanged: _handleTextChange,
                  keyboardType: TextInputType.text,
                  textInputAction: TextInputAction.done,
                  autocorrect: false,
                  enableSuggestions: false,
                  enableInteractiveSelection: false,
                  textCapitalization: TextCapitalization.characters,
                  decoration: const InputDecoration(border: InputBorder.none),
                  style: const TextStyle(fontSize: 1, color: Colors.transparent),
                  cursorColor: Colors.transparent,
                ),
              ),

              Padding(
                padding: EdgeInsets.only(
                  left: 16,
                  right: 16,
                  top: 8,
                  bottom: MediaQuery.of(context).viewInsets.bottom + 8,
                ),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    ElevatedButton.icon(
                      icon: const Icon(Icons.clear_all),
                      label: const Text("Clear Grid"),
                      onPressed: clearGrid,
                    ),
                    ElevatedButton.icon(
                      icon: const Icon(Icons.send),
                      label: const Text("Send Grid (60 chars)"),
                      onPressed: sendGridToESP,
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
