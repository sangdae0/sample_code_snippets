//PiecePuzzleToolEditorWindow.cs
//Auhor: Sangdae Kim
//All content © 2019 DigiPen (USA) Corporation, all rights reserved.

using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using UnityEngine.UI;
using UnityEditor;
using UnityEngine.Sprites;
using System.Text;


public class PiecePuzzleToolEditorWindow : EditorWindow {
  enum ToolMode {
    None,
    CreateNewPuzzleData,
    EditPuzzleData
  }

  const string ImagesSubRootDir = "PiecePuzzleImages/";
  const string ToolRootDir = "Assets/PiecePuzzleTool/Resources/";
  const float DefaultAllowRadius = 0.5f;
  const float DefaultZFightingValue = 0.01f;

  #region Settings
  string splittedImagesPath;
  string puzzleDataPath;
  float zFightingVal = DefaultZFightingValue;
  string fileNameOfPuzzleBoardDataToSave;
  #endregion

  UnityEngine.Object boardObj = null;
  UnityEngine.Object backgroundObj = null;
  GameObject puzzleBoardGo = null;
  GameObject puzzleBackgroundImageGo = null;

  ToolMode currentToolMode;

  [MenuItem("PiecePuzzleTool/EditorWindow")]
  public static void ShowWindow() {
    var editorWindow = GetWindow<PiecePuzzleToolEditorWindow>();
    editorWindow.Init();
  }

  void Init() {
    if (puzzleBoardGo != null)
      puzzleBoardGo.GetComponent<PiecePuzzleBoard>().Clear(isEditor: true);

    splittedImagesPath = string.Empty;
    fileNameOfPuzzleBoardDataToSave = string.Empty;
    boardObj = null;
    backgroundObj = null;
    puzzleBoardGo = null;
    puzzleBackgroundImageGo = null;

    currentToolMode = ToolMode.None;
  }

  private void OnGUI() {
    boardObj = EditorGUILayout.ObjectField("Select object to attach",
                                                  boardObj,
                                                  typeof(UnityEngine.Object),
                                                  allowSceneObjects: true);
    puzzleBoardGo = boardObj as GameObject; 

    switch (currentToolMode) {
    case ToolMode.None:
    OnGUIToolModeNone();
    break;
    case ToolMode.CreateNewPuzzleData:
      OnGUIToolModeCreatingPuzzleData();
    break;
    case ToolMode.EditPuzzleData:
      OnGUIToolModeEditingPuzzleData();
    break;
    default:
    break;
    }
  }

  bool ShowCancelButton() {
    GUILayout.Space(100.0f);
    if (GUILayout.Button("Clear")) {
      Init();
      return true;
    }
    return false;
  }

  void OnGUIToolModeNone() {
    if (GUILayout.Button("Create New Puzzle")) {
      currentToolMode = ToolMode.CreateNewPuzzleData;
    }
    if (GUILayout.Button("Edit Puzzle Data")) {
      puzzleDataPath = EditorUtility.OpenFilePanel("Select puzzle data",
                                                    ToolRootDir,
                                                    "json");
      if (puzzleDataPath == string.Empty) return;
    
      var loadedJson = Resources.Load<TextAsset>(Path.GetFileNameWithoutExtension(puzzleDataPath));
      var loadedPuzzleData = JsonUtility.FromJson<PiecePuzzleBoard.Data>(loadedJson.text);

      puzzleBoardGo.GetComponent<PiecePuzzleBoard>().Init(loadedPuzzleData, fromTool: true);
      currentToolMode = ToolMode.EditPuzzleData;
      return;
    }
  }

  void OnGUIToolModeCreatingPuzzleData() {
    var boardCmp = puzzleBoardGo.GetComponent<PiecePuzzleBoard>();
    if (boardCmp == null)
      boardCmp = puzzleBoardGo.AddComponent<PiecePuzzleBoard>();

    backgroundObj = EditorGUILayout.ObjectField("Background(Not required)",
                                                  backgroundObj,
                                                  typeof(UnityEngine.Object),
                                                  allowSceneObjects: true);
    if (backgroundObj != null) {
      puzzleBackgroundImageGo = backgroundObj as GameObject;
    }

    var zFightingValStr = EditorGUILayout.TextField("Z Fighting Value", zFightingVal.ToString());
    if (zFightingValStr != string.Empty) {
      float parsed;
      if (float.TryParse(zFightingValStr, out parsed))
        zFightingVal = parsed;
    }

    GUILayout.Label("SplittedImagePath: " + splittedImagesPath ?? string.Empty);
    if (GUILayout.Button("Select splitted image path")) {
      splittedImagesPath = EditorUtility.OpenFolderPanel("Select directory of splitted images",
                                                      ToolRootDir + ImagesSubRootDir,
                                                      string.Empty);
    }

    if (!string.IsNullOrEmpty(splittedImagesPath)
        && puzzleBoardGo != null) {
      if (GUILayout.Button("Create New Puzzle")) {
        var resourceDirIdx = splittedImagesPath.IndexOf(ImagesSubRootDir);
        var resourceDir = splittedImagesPath.Substring(resourceDirIdx);
        
        var boardData = new PiecePuzzleBoard.Data() {
          HasBackgroundImage = puzzleBackgroundImageGo ? true : false,
          LocalScale = puzzleBackgroundImageGo ? puzzleBackgroundImageGo.transform.localScale : Vector3.zero,
          PieceList = new List<PuzzlePiece.Data>()
        };

        var boardOffsetV = CalcOffsetV(puzzleBoardGo);
        
        foreach (var imageFile in Directory.GetFiles(splittedImagesPath, "*.png")) {
          var fileName = Path.GetFileNameWithoutExtension(imageFile);
          var textureFilePath = resourceDir + "/" + fileName;
        
          var pieceData = new PuzzlePiece.Data {
            Name = fileName,
            PivotPoint = boardOffsetV,
            TextureFilePath = textureFilePath,
            AllowRadius = DefaultAllowRadius,
            LoadAtFirst = true,
            StartingPos = Vector3.zero,
          };
          boardData.PieceList.Add(pieceData);
        }
        boardCmp.Init(boardData, fromTool: true);
        currentToolMode = ToolMode.EditPuzzleData;
        return;
      }
    }

    if (ShowCancelButton())
      return;
  }

  Vector2 scrollPos;
  float allowRadiusAll;
  bool debugDrawAllowRadiusAll;
  float zPosAll;
  void OnGUIToolModeEditingPuzzleData() {
    const float ElemSpaceOffset = 10.0f;
    var boardCmp = puzzleBoardGo.GetComponent<PiecePuzzleBoard>();

    EditorGUILayout.BeginVertical();
    scrollPos = EditorGUILayout.BeginScrollView(scrollPos,
                                    alwaysShowHorizontal: true,
                                    alwaysShowVertical: true,
                                    GUILayout.Width(500),
                                    GUILayout.ExpandWidth(true),
                                    GUILayout.Height(500),
                                    GUILayout.ExpandHeight(true));
    if (ShowCancelButton())
      return;

    var changedDebugDrawAllowRadiusAll = EditorGUILayout.Toggle("Toggle Draw Allow Radius ALL", debugDrawAllowRadiusAll);
    if (changedDebugDrawAllowRadiusAll != debugDrawAllowRadiusAll) {
      debugDrawAllowRadiusAll = changedDebugDrawAllowRadiusAll;
      foreach (var pieceData in boardCmp.BoardData.PieceList)
        boardCmp.Pieces[pieceData.Name].AllowRadiusDebugDrawEnabled = debugDrawAllowRadiusAll;
    }

    var allowRadiusAllStr = EditorGUILayout.TextField("Change Allow Radius ALL", allowRadiusAll.ToString());
    if (allowRadiusAllStr != string.Empty) {
      float parsed;
      if (float.TryParse(allowRadiusAllStr, out parsed)
        && parsed != allowRadiusAll) {
        allowRadiusAll = parsed;
        foreach (var pieceData in boardCmp.BoardData.PieceList)
          pieceData.AllowRadius = allowRadiusAll;
      }
    }

    var zPosAllStr = EditorGUILayout.TextField("Change Z pos ALL", zPosAll.ToString());
    if (zPosAllStr != string.Empty) {
      float parsed;
      if (float.TryParse(zPosAllStr, out parsed)
        && parsed != zPosAll) {
        zPosAll = parsed;
        foreach (var pieceData in boardCmp.BoardData.PieceList) {
          var prevPos = boardCmp.Pieces[pieceData.Name].transform.localPosition;
          boardCmp.Pieces[pieceData.Name].transform.localPosition = new Vector3(prevPos.x,
                                                                              prevPos.y,
                                                                              zPosAll);
        }
      }
    }
    GUILayout.Space(ElemSpaceOffset);

    if (boardCmp.BoardData.PieceList.Count > 0) {
      if (GUILayout.Button("Move All Pieces to their Pivot Point")) {
        for (int i = 0; i < boardCmp.BoardData.PieceList.Count; ++ i) {
          var pieceGo = boardCmp.Pieces[boardCmp.BoardData.PieceList[i].Name];
          pieceGo.transform.localPosition = boardCmp.BoardData.PieceList[i].PivotPoint;
        }
      }
      GUILayout.Space(ElemSpaceOffset);

      if (GUILayout.Button("Move All Pieces to their Starting Position")) {
        for (int i = 0; i < boardCmp.BoardData.PieceList.Count; ++ i) {
          var pieceGo = boardCmp.Pieces[boardCmp.BoardData.PieceList[i].Name];
          pieceGo.transform.localPosition = boardCmp.BoardData.PieceList[i].StartingPos;
        }
      }
      GUILayout.Space(ElemSpaceOffset * 2);

      if (GUILayout.Button("Set PivotPoint by using Current Position")) {
        for (int i = 0; i < boardCmp.BoardData.PieceList.Count; ++ i) {
          var pieceGo = boardCmp.Pieces[boardCmp.BoardData.PieceList[i].Name];
          boardCmp.BoardData.PieceList[i].PivotPoint = pieceGo.transform.localPosition;
        }
      }
      GUILayout.Space(ElemSpaceOffset);

      if (GUILayout.Button("Set Starting Pos by using Current Position")) {
        for (int i = 0; i < boardCmp.BoardData.PieceList.Count; ++ i) {
          var pieceGo = boardCmp.Pieces[boardCmp.BoardData.PieceList[i].Name];
          boardCmp.BoardData.PieceList[i].StartingPos = pieceGo.transform.localPosition;
        }
      }
      GUILayout.Space(ElemSpaceOffset * 2);

      if (GUILayout.Button("Save Data")) {
        var saveFilePath = EditorUtility.SaveFilePanel(title: "save piece puzzle data",
                                                        directory: ToolRootDir,
                                                        defaultName: string.Empty,
                                                        extension: "json");
        if (saveFilePath != string.Empty) {
          var serialized = JsonUtility.ToJson(boardCmp.BoardData, prettyPrint: true);
          using (FileStream fs = File.Create(saveFilePath)) {
            byte[] info = new UTF8Encoding(true).GetBytes(serialized);
            fs.Write(info, 0, info.Length);
            Debug.Log("File: " + saveFilePath + " is created");
          }
          boardCmp.dataPath = Path.GetFileNameWithoutExtension(saveFilePath);
          boardCmp.Clear(isEditor: true);
          currentToolMode = ToolMode.None;
          return;
        }
      }
    }

    GUILayout.Space(ElemSpaceOffset * 2);

    foreach (var pieceData in boardCmp.BoardData.PieceList) {
      GUILayout.Label(pieceData.Name);
      pieceData.LoadAtFirst = GUILayout.Toggle(pieceData.LoadAtFirst, "LoadAtFirst");
      var pieceCmp = boardCmp.Pieces[pieceData.Name];
      if (pieceCmp == null) continue;

      pieceCmp.AllowRadiusDebugDrawEnabled = EditorGUILayout.Toggle("Draw Allow Radius", pieceCmp.AllowRadiusDebugDrawEnabled);
      var allowRadiusStr = EditorGUILayout.TextField("Allow Radius", pieceData.AllowRadius.ToString());
      if (allowRadiusStr != string.Empty) {
        float parsed;
        if (float.TryParse(allowRadiusStr, out parsed)
          && parsed != pieceData.AllowRadius) {
          pieceData.AllowRadius = parsed;
          var pieceGo = boardCmp.GetPieceGo(pieceData.Name);
          pieceGo.GetComponent<PuzzlePiece>().UpdateAllowRadiusDebugDraw();
        }
      }

      if (GUILayout.Button("Select " + pieceData.Name)) {
        Selection.activeObject = boardCmp.Pieces[pieceData.Name]?.gameObject;
      }

      GUILayout.Space(ElemSpaceOffset);
    }

    foreach (var pieceGo in boardCmp.Pieces) {
      pieceGo.Value.DebugDraw();
    }
  
    EditorGUILayout.EndScrollView();
    EditorGUILayout.EndVertical();
  }

  Vector3 CalcOffsetV(GameObject boardGo) {
    bool isFacePositive = boardGo.transform.forward.x 
                          * boardGo.transform.forward.y
                          * boardGo.transform.forward.z > 0.0f;
    var dpWithX = Vector3.Dot(boardGo.transform.forward,
                                isFacePositive ? new Vector3(1.0f, 0.0f, 0.0f) : new Vector3(-1.0f, 0.0f, 0.0f));
    var dpWithY = Vector3.Dot(boardGo.transform.forward,
                                isFacePositive ? new Vector3(0.0f, 1.0f, 0.0f) : new Vector3(0.0f, -1.0f, 0.0f));
    var dpWithZ = Vector3.Dot(boardGo.transform.forward,
                                isFacePositive ? new Vector3(0.0f, 0.0f, 1.0f) : new Vector3(0.0f, 0.0f, -1.0f));
        
 
    Vector3 offsetV;
    if (dpWithX > 0.0f)
      offsetV = new Vector3(-(boardGo.transform.localScale.x * 0.5f + zFightingVal), 0.0f, 0.0f);
    else if (dpWithY > 0.0f)
      offsetV = new Vector3(0.0f, -(boardGo.transform.localScale.y * 0.5f + zFightingVal), 0.0f);
    else
      offsetV = new Vector3(0.0f, 0.0f, -(boardGo.transform.localScale.z * 0.5f + zFightingVal));
    return offsetV;
  }
}