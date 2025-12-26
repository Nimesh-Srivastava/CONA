import { NextResponse } from 'next/server';
import fs from 'fs';
import path from 'path';

export async function POST(request: Request) {
    try {
        const body = await request.json();
        const { duration = 10, fps = 60, output = 'animation.mp4' } = body;

        const controlPath = path.join(process.cwd(), '..', 'record_control.json');

        const controlData = {
            recording: true,
            duration: parseFloat(duration),
            fps: parseInt(fps),
            output: output
        };

        fs.writeFileSync(controlPath, JSON.stringify(controlData, null, 2));

        return NextResponse.json({ success: true, message: 'Recording started' });
    } catch (error) {
        console.error('Error starting recording:', error);
        return NextResponse.json({ success: false, error: 'Failed to start recording' }, { status: 500 });
    }
}
