'use client';
import { useState, useRef, useEffect } from 'react';

type ShapeType = 'circle' | 'rect' | 'line';

type Shape = {
    id: number;
    type: ShapeType;
    pos: [number, number];
    size: [number, number] | number; // [w, h] for rect, radius for circle
    color: string;
};

type AnimationType = 'translate_x' | 'translate_y' | 'opacity';

type Animation = {
    id: string;
    shapeId: number;
    type: AnimationType;
    start: number;
    end: number;
    startTime: number;
    duration: number;
    loop: boolean;
};

export default function Home() {
    const [shapes, setShapes] = useState<Shape[]>([]);
    const [animations, setAnimations] = useState<Animation[]>([]);

    const [selectedTool, setSelectedTool] = useState<ShapeType>('circle');
    const [selectedColor, setSelectedColor] = useState('#ff0000');
    const [selectedShapeId, setSelectedShapeId] = useState<number | null>(null);

    const canvasRef = useRef<HTMLCanvasElement>(null);
    const [isDrawing, setIsDrawing] = useState(false);
    const [startPos, setStartPos] = useState<[number, number] | null>(null);

    // Export modal state
    const [showExportModal, setShowExportModal] = useState(false);
    const [exportDuration, setExportDuration] = useState(10);
    const [exportFps, setExportFps] = useState(60);
    const [exportFilename, setExportFilename] = useState('animation.mp4');

    // Convert screen coordinates to world/canvas coordinates
    const getPos = (e: React.MouseEvent): [number, number] => {
        if (!canvasRef.current) return [0, 0];
        const rect = canvasRef.current.getBoundingClientRect();
        return [e.clientX - rect.left, e.clientY - rect.top];
    };

    const isHit = (s: Shape, x: number, y: number) => {
        if (s.type === 'circle') {
            const r = typeof s.size === 'number' ? s.size : 10;
            const dx = x - s.pos[0];
            const dy = y - s.pos[1];
            return dx * dx + dy * dy <= r * r;
        } else if (s.type === 'rect') {
            const [w, h] = Array.isArray(s.size) ? s.size : [20, 20];
            return x >= s.pos[0] && x <= s.pos[0] + w && y >= s.pos[1] && y <= s.pos[1] + h;
        }
        // Line hit test omitted for simplicity or standard bounding box
        return false;
    };

    const handleMouseDown = (e: React.MouseEvent) => {
        const pos = getPos(e);

        // Check selection first
        // Reverse iterate to select top-most
        let hitId = null;
        for (let i = shapes.length - 1; i >= 0; i--) {
            if (isHit(shapes[i], pos[0], pos[1])) {
                hitId = shapes[i].id;
                break;
            }
        }

        if (hitId) {
            setSelectedShapeId(hitId);
            return; // Don't draw if we selected something
        } else {
            setSelectedShapeId(null);
        }

        setIsDrawing(true);
        setStartPos(pos);
    };

    const handleMouseUp = (e: React.MouseEvent) => {
        if (!isDrawing || !startPos) return;
        const endPos = getPos(e);
        setIsDrawing(false);

        // Create Shape
        const newShape: Shape = {
            id: Date.now(),
            type: selectedTool,
            pos: startPos,
            size: 0,
            color: selectedColor,
        };

        if (selectedTool === 'circle') {
            const dx = endPos[0] - startPos[0];
            const dy = endPos[1] - startPos[1];
            newShape.size = Math.sqrt(dx * dx + dy * dy);
        } else if (selectedTool === 'rect') {
            newShape.size = [endPos[0] - startPos[0], endPos[1] - startPos[1]];
        } else if (selectedTool === 'line') {
            newShape.size = [endPos[0] - startPos[0], endPos[1] - startPos[1]];
        }

        const updatedShapes = [...shapes, newShape];
        setShapes(updatedShapes);
        saveScene(updatedShapes, animations);
    };

    const saveScene = async (currentShapes: Shape[], currentAnims: Animation[]) => {
        await fetch('/api/save', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ shapes: currentShapes, animations: currentAnims }),
        });
    };

    const addAnimation = () => {
        if (!selectedShapeId) return;
        const newAnim: Animation = {
            id: `anim_${Date.now()}`,
            shapeId: selectedShapeId,
            type: 'translate_x',
            start: 0,
            end: 100,
            startTime: 0,
            duration: 2,
            loop: true
        };
        const updatedAnims = [...animations, newAnim];
        setAnimations(updatedAnims);
        saveScene(shapes, updatedAnims);
    };

    const updateAnimation = (id: string, field: keyof Animation, value: any) => {
        const updated = animations.map(a => a.id === id ? { ...a, [field]: value } : a);
        setAnimations(updated);
        saveScene(shapes, updated);
    };

    const deleteAnimation = (id: string) => {
        const updated = animations.filter(a => a.id !== id);
        setAnimations(updated);
        saveScene(shapes, updated);
    };

    const startExport = async () => {
        try {
            await fetch('/api/record', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    duration: exportDuration,
                    fps: exportFps,
                    output: exportFilename
                }),
            });
            setShowExportModal(false);
            alert(`Recording started! Check the C engine window. Video will be saved as ${exportFilename}`);
        } catch (error) {
            console.error('Export failed:', error);
            alert('Failed to start export');
        }
    };

    // Render Canvas
    useEffect(() => {
        const canvas = canvasRef.current;
        if (!canvas) return;
        const ctx = canvas.getContext('2d');
        if (!ctx) return;

        ctx.fillStyle = '#1e1e1e';
        ctx.fillRect(0, 0, canvas.width, canvas.height);

        // Draw Grid
        ctx.strokeStyle = '#333';
        ctx.lineWidth = 1;
        for (let i = 0; i < canvas.width; i += 50) { ctx.beginPath(); ctx.moveTo(i, 0); ctx.lineTo(i, canvas.height); ctx.stroke(); }
        for (let i = 0; i < canvas.height; i += 50) { ctx.beginPath(); ctx.moveTo(0, i); ctx.lineTo(canvas.width, i); ctx.stroke(); }

        shapes.forEach(s => {
            ctx.save();
            if (s.id === selectedShapeId) {
                ctx.strokeStyle = '#00ffff';
                ctx.lineWidth = 4;
                ctx.shadowColor = '#00ffff';
                ctx.shadowBlur = 10;
            } else {
                ctx.strokeStyle = s.color;
                ctx.lineWidth = 2;
            }

            ctx.fillStyle = s.color;

            if (s.type === 'circle') {
                ctx.beginPath();
                const r = typeof s.size === 'number' ? s.size : 10;
                ctx.arc(s.pos[0], s.pos[1], r, 0, Math.PI * 2);
                ctx.fill();
                ctx.stroke();
            } else if (s.type === 'rect') {
                const [w, h] = Array.isArray(s.size) ? s.size : [20, 20];
                ctx.fillRect(s.pos[0], s.pos[1], w, h);
                ctx.strokeRect(s.pos[0], s.pos[1], w, h);
            } else if (s.type === 'line') {
                const [dx, dy] = Array.isArray(s.size) ? s.size : [20, 20];
                ctx.beginPath();
                ctx.moveTo(s.pos[0], s.pos[1]);
                ctx.lineTo(s.pos[0] + dx, s.pos[1] + dy);
                ctx.stroke();
            }
            ctx.restore();
        });

    }, [shapes, selectedShapeId]);

    return (
        <main className="flex min-h-screen flex-row bg-black text-white">
            {/* Sidebar */}
            <div className="w-64 border-r border-gray-800 p-4 flex flex-col gap-4">
                <h1 className="text-xl font-bold bg-gradient-to-r from-blue-400 to-purple-500 bg-clip-text text-transparent">Animation Studio</h1>

                <div className="flex flex-col gap-2">
                    <label className="text-sm text-gray-400">Tools</label>
                    <div className="flex gap-2">
                        <button onClick={() => setSelectedTool('circle')} className={`p-2 rounded ${selectedTool === 'circle' ? 'bg-blue-600' : 'bg-gray-800'}`}>O</button>
                        <button onClick={() => setSelectedTool('rect')} className={`p-2 rounded ${selectedTool === 'rect' ? 'bg-blue-600' : 'bg-gray-800'}`}>□</button>
                        <button onClick={() => setSelectedTool('line')} className={`p-2 rounded ${selectedTool === 'line' ? 'bg-blue-600' : 'bg-gray-800'}`}>/</button>
                    </div>
                </div>

                <div className="flex flex-col gap-2">
                    <label className="text-sm text-gray-400">Color</label>
                    <input type="color" value={selectedColor} onChange={e => setSelectedColor(e.target.value)} className="w-full h-10 bg-transparent cursor-pointer" />
                </div>

                <div className="mt-auto flex flex-col gap-2">
                    <button onClick={() => setShowExportModal(true)} className="w-full p-2 bg-green-900/50 hover:bg-green-800 rounded text-green-200">📹 Export Video</button>
                    <button onClick={() => { setShapes([]); setAnimations([]); saveScene([], []); }} className="w-full p-2 bg-red-900/50 hover:bg-red-800 rounded text-red-200">Clear Scene</button>
                </div>
            </div>

            {/* Canvas Area */}
            <div className="flex-1 relative overflow-hidden flex items-center justify-center bg-[#101010]">
                <canvas
                    ref={canvasRef}
                    width={800}
                    height={600}
                    className="bg-[#1e1e1e] shadow-2xl border border-gray-800 cursor-crosshair"
                    onMouseDown={handleMouseDown}
                    onMouseUp={handleMouseUp}
                />
                <div className="absolute top-4 right-4 bg-black/50 p-2 rounded text-xs text-gray-500">
                    Canvas: 800x600
                </div>
            </div>

            {/* Properties Panel (Right) */}
            <div className="w-72 border-l border-gray-800 p-4 flex flex-col gap-4 bg-[#0a0a0a]">
                <h2 className="text-lg font-bold">Properties</h2>
                {selectedShapeId ? (
                    <div className="flex flex-col gap-4">
                        <div className="text-sm text-gray-400">Selected Shape ID: {selectedShapeId}</div>

                        <div className="border-t border-gray-800 pt-4">
                            <div className="flex justify-between items-center mb-2">
                                <h3 className="font-semibold">Animations</h3>
                                <button onClick={addAnimation} className="text-xs bg-blue-600 px-2 py-1 rounded hover:bg-blue-500">+ Add</button>
                            </div>

                            <div className="flex flex-col gap-3 max-h-[500px] overflow-y-auto">
                                {animations.filter(a => a.shapeId === selectedShapeId).map(anim => (
                                    <div key={anim.id} className="bg-gray-900 p-3 rounded border border-gray-800 text-sm">
                                        <div className="flex justify-between mb-2">
                                            <select
                                                value={anim.type}
                                                onChange={(e) => updateAnimation(anim.id, 'type', e.target.value)}
                                                className="bg-black border border-gray-700 rounded px-1"
                                            >
                                                <option value="translate_x">Translate X</option>
                                                <option value="translate_y">Translate Y</option>
                                                <option value="opacity">Opacity</option>
                                            </select>
                                            <button onClick={() => deleteAnimation(anim.id)} className="text-red-500 hover:text-red-400">×</button>
                                        </div>

                                        <div className="grid grid-cols-2 gap-2">
                                            <div>
                                                <label className="text-xs text-gray-500">Start</label>
                                                <input type="number" value={anim.start} onChange={e => updateAnimation(anim.id, 'start', parseFloat(e.target.value) || 0)} className="w-full bg-black border border-gray-700 rounded px-1" />
                                            </div>
                                            <div>
                                                <label className="text-xs text-gray-500">End</label>
                                                <input type="number" value={anim.end} onChange={e => updateAnimation(anim.id, 'end', parseFloat(e.target.value) || 0)} className="w-full bg-black border border-gray-700 rounded px-1" />
                                            </div>
                                            <div>
                                                <label className="text-xs text-gray-500">Duration (s)</label>
                                                <input type="number" value={anim.duration} onChange={e => updateAnimation(anim.id, 'duration', parseFloat(e.target.value) || 0)} className="w-full bg-black border border-gray-700 rounded px-1" />
                                            </div>
                                            <div>
                                                <label className="text-xs text-gray-500">Start Time</label>
                                                <input type="number" value={anim.startTime} onChange={e => updateAnimation(anim.id, 'startTime', parseFloat(e.target.value) || 0)} className="w-full bg-black border border-gray-700 rounded px-1" />
                                            </div>
                                        </div>
                                    </div>
                                ))}
                                {animations.filter(a => a.shapeId === selectedShapeId).length === 0 && (
                                    <div className="text-gray-600 italic text-xs">No animations</div>
                                )}
                            </div>
                        </div>
                    </div>
                ) : (
                    <div className="text-gray-500 text-sm mt-10 text-center">Select a shape to edit properties</div>
                )}
            </div>

            {/* Export Modal */}
            {showExportModal && (
                <div className="fixed inset-0 bg-black/70 flex items-center justify-center z-50">
                    <div className="bg-gray-900 border border-gray-700 rounded-lg p-6 w-96">
                        <h2 className="text-xl font-bold mb-4">Export Video</h2>

                        <div className="flex flex-col gap-4">
                            <div>
                                <label className="text-sm text-gray-400">Duration (seconds)</label>
                                <input
                                    type="number"
                                    value={exportDuration}
                                    onChange={e => setExportDuration(parseFloat(e.target.value))}
                                    className="w-full bg-black border border-gray-700 rounded px-3 py-2 mt-1"
                                    min="1"
                                    max="60"
                                />
                            </div>

                            <div>
                                <label className="text-sm text-gray-400">FPS</label>
                                <input
                                    type="number"
                                    value={exportFps}
                                    onChange={e => setExportFps(parseInt(e.target.value))}
                                    className="w-full bg-black border border-gray-700 rounded px-3 py-2 mt-1"
                                    min="24"
                                    max="120"
                                />
                            </div>

                            <div>
                                <label className="text-sm text-gray-400">Filename</label>
                                <input
                                    type="text"
                                    value={exportFilename}
                                    onChange={e => setExportFilename(e.target.value)}
                                    className="w-full bg-black border border-gray-700 rounded px-3 py-2 mt-1"
                                />
                            </div>

                            <div className="flex gap-2 mt-4">
                                <button
                                    onClick={startExport}
                                    className="flex-1 bg-green-600 hover:bg-green-700 rounded px-4 py-2"
                                >
                                    Start Recording
                                </button>
                                <button
                                    onClick={() => setShowExportModal(false)}
                                    className="flex-1 bg-gray-700 hover:bg-gray-600 rounded px-4 py-2"
                                >
                                    Cancel
                                </button>
                            </div>
                        </div>
                    </div>
                </div>
            )}
        </main>
    );
}

