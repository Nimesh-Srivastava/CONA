import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
    title: "Animation Studio",
    description: "Web-based Animation Studio",
};

export default function RootLayout({
    children,
}: Readonly<{
    children: React.ReactNode;
}>) {
    return (
        <html lang="en">
            <body>{children}</body>
        </html>
    );
}
