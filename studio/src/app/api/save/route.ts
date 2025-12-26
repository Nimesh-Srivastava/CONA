import { NextResponse } from 'next/server';
import fs from 'fs';
import path from 'path';

export async function POST(request: Request) {
    try {
        const data = await request.json();
        // Save to parent directory of 'studio' (which is the project root)
        // process.cwd() is .../animation-engine/studio
        const filePath = path.join(process.cwd(), '..', 'scene.json');
        fs.writeFileSync(filePath, JSON.stringify(data, null, 2));
        return NextResponse.json({ success: true });
    } catch (e) {
        console.error(e);
        return NextResponse.json({ success: false, error: String(e) }, { status: 500 });
    }
}
