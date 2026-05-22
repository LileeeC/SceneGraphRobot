## **Report**

### 1. Environment

專案採用跨平台的 CMake 工具進行建置管線配置，相容 Windows 11 下的 MSVC 環境：

| 環境類別 | 具體技術規格 |
| --- | --- |
| **作業系統 (OS)** | Windows 11 Home / Pro |
| **開發工具 (IDE)** | Visual Studio Code / Visual Studio 2022 |
| **編譯器 (Compiler)** | MSVC (amd64) 編譯器，支援 C++ 17 / C++ 20 標準 |
| **建置工具 (Build Tool)** | CMake (Minimum required version 3.10) |
| **核心圖學底層庫** | GLFW (視窗、上下文與輸入事件)、GLAD (OpenGL 3.3 函式指標載入器) |
| **圖學數學運算庫** | GLM (OpenGL Mathematics, v0.9.9.8) — 全面 Column-Major 矩陣運算 |

---

### 2. Method Description

#### 2.1 階層式場景圖架構 (Hierarchical Scene Graph)

為了組裝具備運動親代關聯的 3D 機器人，專案實作了 `SceneNode` 節點類別與管理整體的 `Robot` 類別。樹狀階層的關節樹深度達到 **5 層**，完整骨架規劃如下：

```
root (世界錨點，不繪製，負責全域運動)
 └── body (軀幹，深鋼鐵灰)
      ├── head (頭部，高科技亮黃)
      ├── leftUpperArm ── leftLowerArm (雙臂，機械淺灰)
      ├── rightUpperArm ── rightLowerArm
      ├── leftUpperLeg (大腿，碳纖灰) ── leftLowerLeg (小腿，機械灰) ── leftFoot (腳掌，警示鮮紅)
      └── rightUpperLeg ── rightLowerLeg ── rightFoot
```

#### 關鍵設計決策：`meshScale` 與 `localTransform` 的徹底分離

這是 3D 場景圖開發中最容易踩到的變形大坑。在場景樹遞迴向下刷新時，子節點會不斷承接父節點的世界矩陣。若將各肢體的「形狀伸縮（Scale）」混入 `localTransform` 中，當父節點（如 Body）進行非等比伸縮時，該變形會連帶污染到所有子節點，使大腿小腿一起被壓扁或拉長。

專案將控制方塊網格形狀的 `meshScale` 獨立抽離，**使其只在最終 `drawNode` 渲染時作用在頂點最右側，絕對不向下串聯傳遞**。

- **場景樹世界矩陣遞迴公式**（行主序 Column-Major，由右至左執行）：
    
    $$globalTransform_{child} = globalTransform_{parent} \times localTransform_{child}$$
    
- **最終頂點渲染 MVP 複合變換矩陣**：
    
    $$MVP = viewProjection \times globalTransform \times meshScale$$
    

#### 2.2 高階 Blinn-Phong 光照渲染著色器

為了徹底打破單色輸出（Flat Color）在關閉線框模式後的 2D 扁平感，本專案將 VBO 頂點 Stride 由原本的 3 floats (僅位置) 升級為 **6 floats/vertex**，資料格式為 `[px, py, pz, nx, ny, nz]`，全面補入 3D 立方體各表面的法向量（Normals）。

- **Normal Matrix (法向量修正矩陣)**：當機器人的 `meshScale` 包含非等比縮放（如扁平扁長的腳掌 0.2×0.1×0.35）時，若直接用 Model 矩陣去變換法向量，會導致法向量被扭曲偏斜、使立體光影破碎。本專案在 CPU 端為每個節點動態計算法向量矩陣，完美維持光影正確性：
    
    $NormalMatrix = \text{transpose}(\text{inverse}(globalTransform \times meshScale))$
    
- **Blinn-Phong 光照公式**：在 Fragment Shader 內部引入半角向量 $H = \text{normalize}(L + V)$，並實作環境光、漫反射與鏡面高光，末端更加入 Tone-mapping 與 Gamma 2.2 修正，使金屬高光呈現精緻的晶瑩漸層：
    
    $\text{Result} = \text{Ambient}(0.18 \times \text{Color}) + \text{Diffuse}(\max(N \cdot L, 0) \times \text{Color}) + \text{Specular}(\max(N \cdot H, 0)^{32.0} \times 0.35)$
    

#### 2.3 多狀態非線性動畫狀態機 (State Machine)

動畫透過 `RobotMode` 列舉配合 `setMode(mode, currentTime)` 管理，切換狀態時傳入全域時間以重置局部計時器（`localTime`），有效防止多關節角度在大跨度切換時發生畫面跳變。

1. **`IDLE` (待機呼吸模式)**：
    
    身體（Body）隨時間進行微弱的 Y 軸正弦浮動： $body.Y = 0.015 \times \sin(2\pi \times 1.2t)$。同時頭部輕微繞 Z 軸左右側傾、雙臂微擺，模擬真實角色的待機呼吸動態。
    
2. **`WALK` (圓軌跑步模式 — 物理連動加分項)**：
    - **公轉運動與潮汐鎖定**：利用極座標公式讓 Root 節點在半徑 $R = 1.8$ 的世界軌道上奔跑（ $x = R\cos\theta, z = R\sin\theta$），並將面向角度修正為 `facingAngle = -orbitAngle`。配合切線向量修正，實現自轉與公轉完美同步，達成永遠面朝前進方向的「潮汐鎖定」效果。
    - **向心力物理內傾（Centripetal Lean）**：模擬跑者對抗向心力的真實動態，在 Body 節點上強制疊加 4 度的 Z 軸側傾旋轉，呈現如同重機壓車一般的極致逼真物理連動。
    - **膝關節生物力學連動（Knee Coupling）**：利用 `glm::clamp` 函數約束小腿角度，大腿向後擺盪時（ $\theta_{upper} < 0$）小腿自動向後折疊以利離地；大腿向前跨出時小腿自動伸直，完美契合生物行走力學。
3. **`KICK` (高階非線性正踢模式)**：
    
    利用純數學三次曲線 $f(x) = 3x^2 - 2x^3$ (`smoothstep`) 來取代死板的線性插值，在 2.0 秒的循環內劃分四個獨立影格時間段：**「25% 慢速後擺蓄力** $\rightarrow$ **20% 大腿高速前甩** $\rightarrow$ **10% 小腿在最高點瞬間爆發彈直** $\rightarrow$ **45% 緩慢平滑收腿」**，展現極具速度張力的大腳正踢動作。
    

#### 2.4 方向感知攝影機控制器 (Direction-Aware Camera)

每幀在主循環中即時利用向量外積動態計算法向量與橫移軸向：

```cpp
glm::vec3 forward = glm::normalize(cameraTarget - cameraPos);
glm::vec3 right   = glm::normalize(glm::cross(forward, cameraUp));
```

相機移動不再被世界死板鎖定，而是能完美感知當前視角。按上下方向鍵時相機順著視線精準推進/拉遠，按左右方向鍵時相機沿著切線軌道流暢環繞，操作極具大作質感。

### 3. 如何執行程式 (How to Run the Program)

#### 3.1 建置與編譯

本專案不需配置繁瑣的環境，請於專案根目錄（與 `CMakeLists.txt` 同層）依序輸入以下標準指令：

```bash
# 1. 建立建置目錄
mkdir build
cd build

# 2. 執行 CMake 專案配置
cmake ..

# 3. 呼叫編譯器進行 Debug 構建 (Windows 下會自動啟用 MSBuild 編譯出 .exe)
cmake --build . --config Debug
```

#### 3.2 鍵盤即時操作說明

編譯完成後啟動執行檔，會彈出一個 1024x768 的高清 3D 視窗。請切換為**英文輸入法**並點擊聚焦該視窗：

- **數字鍵 `1`**：切換為 **IDLE 待機呼吸模式**（原地上下起伏、雙臂微擺）。
- **數字鍵 `2`**：切換為 **WALK 圓軌跑步模式**（面朝前方繞圈跑步、身體向內側物理壓車傾斜、膝蓋自然連動折疊）。
- **數字鍵 `3`**：切換為 **KICK 高階正踢模式**（分段非線性爆發踢球動作）。
- **鍵盤方向鍵 `↑` / `↓`**：順著相機視線前進推進（Dolly In）或向後拉遠（Dolly Out）。
- **鍵盤方向鍵 `←` / `→`**：依當前相對視角精準進行左右環繞橫移（Strafe Left / Right）。
- **按鍵 `ESC`**：關閉 3D 視窗並安全釋放所有動態場景樹節點記憶體。