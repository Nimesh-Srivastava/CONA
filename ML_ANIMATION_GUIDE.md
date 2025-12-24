# Machine Learning Animation Guide (3D Digit Training)

This guide describes the **3D Digit Recognition Visualization** implemented in `plug.c`. It simulates the training process of a neural network learning to classify handwritten digits (0-9).

## 1. Architecture

The network is visualized in 3D space with the following topology:

-   **Input Layer**: 8x8 Grid (64 neurons). Represents the pixels of the input image.
-   **Hidden Layers**: 2 layers of 16 neurons each. Represent the feature extraction capability.
-   **Output Layer**: 10 neurons. Represent classes 0-9.

## 2. Training Loop State Machine

The visualization cycles through 4 distinct phases to simulate a training step:

### Phase 1: INPUTTING DATA (`STATE_INPUT`)
-   An 8x8 bitmap for the current digit (hardcoded in `FONT_DIGITS`) is loaded into the input grid.
-   Active pixels light up Blue.

### Phase 2: FORWARD PROPAGATION (`STATE_PROPAGATE`)
-   Signals travel from Input -> Hidden -> Output layers.
-   Visualized as Gold particles moving along synaptic connections.
-   Timing simulates the computation delay of inference.

### Phase 3: CALCULATING LOSS (`STATE_OUTPUT`)
-   The network produces a prediction.
-   Output neurons light up based on inferred confidence.

### Phase 4: BACKPROPAGATION (`STATE_LEARN`)
-   The "Correct" digit is highlighted in Green.
-   Incorrect high-confidence predictions (errors) are highlighted in Red.
-   This visualizes the "Gradient Descent" step where weights are adjusted (implied).

## 3. Rendering Implementation

### 3D Projection
We use `BeginMode3D` with an orbital camera to allow the user to inspect the network from all angles.

### Dynamic Layers
`init_network` calculates the X,Y,Z positions for neurons:
-   **Input**: `Z = -10.0`, Arranged in a grid.
-   **Hidden**: `Z = -2.0` and `6.0`, Arranged in columns.
-   **Output**: `Z = 14.0`, Arranged in a row.

### Audio Sync
-   Volume is kept very low for ambient effect.
-   Frequency shifts during phases:
    -   **Input**: 220Hz (Low Ping)
    -   **Propagate**: 440Hz (High Ping)
    -   **Learn**: 110Hz (Bass Pulse)

## 4. How to Extend
-   **New Fonts**: Add more 8x8 arrays to `FONT_DIGITS`.
-   **More Layers**: Update `TOTAL_LAYERS` and `init_network` logic.
-   **Real Model**: Load weights from a file and use them to set synapse opacity.
