#pragma once
// routeExport.hpp — Vector PDF Generator for Uttarakhand Disaster Rescue
//
// Generates an emergency-grade, high-resolution vector PDF route sheet.
// It directly outputs standard PDF-1.4 vector drawing instructions:
//   - Full Uttarakhand geographic road graph (NH highways & mountain roads)
//   - Active rescue route highlighted with moving-path directional arrows
//   - High-contrast emergency start/dest pins & numbered sequence nodes
//   - Multi-metric mission summary cards (ETA, Distance, Risk score, Algorithm)
//   - Turn-by-turn waypoint itinerary manifest with field status checkboxes
//   - Completely self-contained, no external PDF library required.

#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <set>
#include <ctime>
#include "graph.hpp"

inline std::string pdfEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '(' || c == ')' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}

inline std::string pdfCircle(double cx, double cy, double r, bool fill = true, bool stroke = true) {
    double k = 0.55228475 * r;
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << cx << " " << (cy + r) << " m "
       << (cx + k) << " " << (cy + r) << " " << (cx + r) << " " << (cy + k) << " " << (cx + r) << " " << cy << " c "
       << (cx + r) << " " << (cy - k) << " " << (cx + k) << " " << (cy - r) << " " << cx << " " << (cy - r) << " c "
       << (cx - k) << " " << (cy - r) << " " << (cx - r) << " " << (cy - k) << " " << (cx - r) << " " << cy << " c "
       << (cx - r) << " " << (cy + k) << " " << (cx - k) << " " << (cy + r) << " " << cx << " " << (cy + r) << " c ";
    if (fill && stroke) ss << "B\n";
    else if (fill) ss << "f\n";
    else ss << "S\n";
    return ss.str();
}

inline std::string pdfArrow(double x1, double y1, double x2, double y2, double pos = 0.52, double size = 5.5) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    double len = std::sqrt(dx * dx + dy * dy);
    if (len < 12.0) return "";
    double ux = dx / len;
    double uy = dy / len;
    double nx = -uy;
    double ny = ux;

    double tx = x1 + dx * pos;
    double ty = y1 + dy * pos;
    double w1x = tx - ux * size + nx * (size * 0.50);
    double w1y = ty - uy * size + ny * (size * 0.50);
    double w2x = tx - ux * size - nx * (size * 0.50);
    double w2y = ty - uy * size - ny * (size * 0.50);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << tx << " " << ty << " m "
       << w1x << " " << w1y << " l "
       << (tx - ux * (size * 0.70)) << " " << (ty - uy * (size * 0.70)) << " l "
       << w2x << " " << w2y << " l h f\n";
    return ss.str();
}

inline std::string pdfText(const std::string& font, double size, double r, double g, double b,
                           double x, double y, const std::string& text) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "BT /" << font << " " << size << " Tf "
       << r << " " << g << " " << b << " rg "
       << x << " " << y << " Td (" << pdfEscape(text) << ") Tj ET\n";
    return ss.str();
}

inline std::string buildRoutePdf(
    const Graph& graph,
    const std::string& startId, const std::string& endId,
    const std::string& algorithmLabel,
    const std::vector<std::string>& pathIds,
    double etaMinutes, double totalDistanceKm, double cost, bool hasCost)
{
    // Page dimensions: A4 Landscape (842 x 595 points)
    const double pageW = 842.0;
    const double pageH = 595.0;
    (void)pageW; (void)pageH;

    std::ostringstream c;
    c << std::fixed << std::setprecision(2);

    // ── 1. Top Header Banner ──────────────────────────────────────
    c << "0.06 0.09 0.16 rg 0 540 842 55 re f\n";          // Dark navy banner
    c << "0.85 0.15 0.15 rg 0 537 842 3 re f\n";           // Red accent line

    c << pdfText("F2", 14, 1.0, 1.0, 1.0, 25, 568, "UTTARAKHAND DISASTER MANAGEMENT AUTHORITY");
    c << pdfText("F1", 9, 0.85, 0.88, 0.95, 25, 550, "EMERGENCY RESCUE ROUTE MANIFEST — MOVING PATH & TOPOLOGICAL GRAPH");

    // Timestamp & Mission ID
    std::time_t now = std::time(nullptr);
    char timeBuf[64];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M UTC", std::gmtime(&now));
    c << pdfText("F1", 8, 0.75, 0.80, 0.88, 620, 568, std::string("Generated: ") + timeBuf);
    c << pdfText("F2", 8, 0.95, 0.77, 0.06, 620, 550, "OP STATUS: ACTIVE EMERGENCY DISPATCH");

    // ── 2. Calculate Geographic Bounds for Uttarakhand ───────────
    double minLat = 999.0, maxLat = -999.0, minLng = 999.0, maxLng = -999.0;
    for (const auto& [id, n] : graph.nodes) {
        if (n.lat < minLat) minLat = n.lat;
        if (n.lat > maxLat) maxLat = n.lat;
        if (n.lng < minLng) minLng = n.lng;
        if (n.lng > maxLng) maxLng = n.lng;
    }
    // Small padding
    minLat -= 0.08; maxLat += 0.08;
    minLng -= 0.08; maxLng += 0.08;

    // Map drawing container box on PDF
    const double mapX = 25.0;
    const double mapY = 30.0;
    const double mapW = 475.0;
    const double mapH = 490.0;

    // Draw Map Frame & Background
    c << "0.96 0.97 0.99 rg " << mapX << " " << mapY << " " << mapW << " " << mapH << " re f\n";
    c << "0.5 w 0.80 0.84 0.90 RG " << mapX << " " << mapY << " " << mapW << " " << mapH << " re S\n";

    // Cartographic aspect ratio scaling (longitude shrunk by cos(latitude))
    double midLatRad = (minLat + maxLat) * 0.5 * 3.141592653589793 / 180.0;
    double latSpanKm = (maxLat - minLat) * 111.0;
    double lngSpanKm = (maxLng - minLng) * (111.0 * std::cos(midLatRad));
    double aspect = lngSpanKm / latSpanKm;

    double drawW = mapW - 30.0;
    double drawH = drawW / aspect;
    if (drawH > mapH - 35.0) {
        drawH = mapH - 35.0;
        drawW = drawH * aspect;
    }
    double offX = mapX + (mapW - drawW) * 0.5;
    double offY = mapY + (mapH - drawH) * 0.5;

    auto proj = [&](double lat, double lng) -> std::pair<double, double> {
        double nx = (lng - minLng) / (maxLng - minLng);
        double ny = (lat - minLat) / (maxLat - minLat);
        return { offX + nx * drawW, offY + ny * drawH };
    };

    // Map Sub-header / Compass
    c << pdfText("F2", 8.5, 0.20, 0.25, 0.35, mapX + 12, mapY + mapH - 18, "TOPOLOGICAL ROAD NETWORK & LIVE PATH");
    c << pdfText("F1", 7, 0.45, 0.50, 0.58, mapX + 12, mapY + mapH - 28, "Coordinate space: 28.9N-31.1N, 77.8E-80.3E (Garhwal & Kumaon)");
    // North Indicator
    c << "0.15 0.20 0.30 RG 1 w "
      << (mapX + mapW - 25) << " " << (mapY + mapH - 30) << " m "
      << (mapX + mapW - 25) << " " << (mapY + mapH - 12) << " l S\n";
    c << "0.15 0.20 0.30 rg "
      << (mapX + mapW - 25) << " " << (mapY + mapH - 10) << " m "
      << (mapX + mapW - 28) << " " << (mapY + mapH - 16) << " l "
      << (mapX + mapW - 22) << " " << (mapY + mapH - 16) << " l h f\n";
    c << pdfText("F2", 6.5, 0.15, 0.20, 0.30, mapX + mapW - 27.5, mapY + mapH - 8, "N");

    // Set of active path consecutive edges
    std::set<std::pair<std::string, std::string>> activeEdges;
    for (size_t i = 0; i + 1 < pathIds.size(); i++) {
        activeEdges.insert({pathIds[i], pathIds[i+1]});
        activeEdges.insert({pathIds[i+1], pathIds[i]});
    }

    // ── 3. Draw Base Road Graph (Inactive Network) ───────────────
    c << "0.8 w 0.82 0.85 0.90 RG 1 J 1 j\n";
    std::set<std::pair<std::string, std::string>> drawnEdges;
    for (const auto& [u, edges] : graph.adj) {
        if (graph.nodes.find(u) == graph.nodes.end()) continue;
        auto [x1, y1] = proj(graph.nodes.at(u).lat, graph.nodes.at(u).lng);
        for (const auto& e : edges) {
            std::string v = e.to;
            if (graph.nodes.find(v) == graph.nodes.end()) continue;
            std::pair<std::string, std::string> edgeKey = u < v ? std::make_pair(u, v) : std::make_pair(v, u);
            if (drawnEdges.count(edgeKey)) continue;
            drawnEdges.insert(edgeKey);

            if (activeEdges.count({u, v})) continue; // Drawn later with highlight

            auto [x2, y2] = proj(graph.nodes.at(v).lat, graph.nodes.at(v).lng);
            if (e.blocked) {
                c << "[2 2] 0 d 0.90 0.40 0.35 RG 1.1 w " << x1 << " " << y1 << " m " << x2 << " " << y2 << " l S [] 0 d\n";
                c << "0.8 w 0.82 0.85 0.90 RG\n";
            } else {
                c << x1 << " " << y1 << " m " << x2 << " " << y2 << " l S\n";
            }
        }
    }

    // ── 4. Draw Active Rescue Route (The Moving Path) ────────────
    if (pathIds.size() >= 2) {
        // Outer halo / glow for route
        c << "5.5 w 0.58 0.78 0.98 RG 1 J 1 j\n";
        for (size_t i = 0; i + 1 < pathIds.size(); i++) {
            if (!graph.nodes.count(pathIds[i]) || !graph.nodes.count(pathIds[i+1])) continue;
            auto [x1, y1] = proj(graph.nodes.at(pathIds[i]).lat, graph.nodes.at(pathIds[i]).lng);
            auto [x2, y2] = proj(graph.nodes.at(pathIds[i+1]).lat, graph.nodes.at(pathIds[i+1]).lng);
            c << x1 << " " << y1 << " m " << x2 << " " << y2 << " l S\n";
        }

        // Inner solid route line
        c << "2.8 w 0.12 0.38 0.88 RG 1 J 1 j\n";
        for (size_t i = 0; i + 1 < pathIds.size(); i++) {
            if (!graph.nodes.count(pathIds[i]) || !graph.nodes.count(pathIds[i+1])) continue;
            auto [x1, y1] = proj(graph.nodes.at(pathIds[i]).lat, graph.nodes.at(pathIds[i]).lng);
            auto [x2, y2] = proj(graph.nodes.at(pathIds[i+1]).lat, graph.nodes.at(pathIds[i+1]).lng);
            c << x1 << " " << y1 << " m " << x2 << " " << y2 << " l S\n";
        }

        // Directional Moving-Path Arrows (Chevrons pointing in direction of travel)
        c << "0.96 0.62 0.04 rg\n"; // Bold amber chevrons
        for (size_t i = 0; i + 1 < pathIds.size(); i++) {
            if (!graph.nodes.count(pathIds[i]) || !graph.nodes.count(pathIds[i+1])) continue;
            auto [x1, y1] = proj(graph.nodes.at(pathIds[i]).lat, graph.nodes.at(pathIds[i]).lng);
            auto [x2, y2] = proj(graph.nodes.at(pathIds[i+1]).lat, graph.nodes.at(pathIds[i+1]).lng);
            c << pdfArrow(x1, y1, x2, y2, 0.50, 6.0);
        }
    }

    // ── 5. Draw Graph Nodes ──────────────────────────────────────
    std::set<std::string> pathNodeSet(pathIds.begin(), pathIds.end());

    // A. Background non-route nodes
    for (const auto& [id, n] : graph.nodes) {
        if (pathNodeSet.count(id)) continue;
        auto [nx, ny] = proj(n.lat, n.lng);
        c << "0.5 w 1.0 1.0 1.0 RG 0.62 0.68 0.76 rg " << pdfCircle(nx, ny, 2.8, true, true);
        c << pdfText("F1", 5.5, 0.45, 0.50, 0.58, nx + 4.5, ny - 2, n.label + " (" + id + ")");
    }

    // B. Active Path Intermediate Nodes (Numbered Sequenced Pins)
    for (size_t i = 1; i + 1 < pathIds.size(); i++) {
        std::string id = pathIds[i];
        if (!graph.nodes.count(id)) continue;
        const auto& n = graph.nodes.at(id);
        auto [nx, ny] = proj(n.lat, n.lng);

        c << "1.0 w 1.0 1.0 1.0 RG 0.14 0.44 0.91 rg " << pdfCircle(nx, ny, 5.5, true, true);
        std::string seqStr = std::to_string(i);
        double tOffX = i < 10 ? -2.2 : -4.0;
        c << pdfText("F2", 6.5, 1.0, 1.0, 1.0, nx + tOffX, ny - 2.2, seqStr);
        c << pdfText("F2", 7.0, 0.12, 0.20, 0.35, nx + 7.5, ny - 2.0, n.label);
    }

    // C. Start Pin (Emerald Green)
    if (!pathIds.empty() && graph.nodes.count(pathIds.front())) {
        const auto& sn = graph.nodes.at(pathIds.front());
        auto [sx, sy] = proj(sn.lat, sn.lng);
        // Outer halo
        c << "0.10 0.65 0.30 rg " << pdfCircle(sx, sy, 8.5, true, false);
        c << "1.5 w 1.0 1.0 1.0 RG 0.08 0.55 0.24 rg " << pdfCircle(sx, sy, 6.5, true, true);
        c << pdfText("F2", 7.0, 1.0, 1.0, 1.0, sx - 2.8, sy - 2.5, "S");
        // Start Badge Label
        c << "0.08 0.55 0.24 rg " << (sx + 9) << " " << (sy - 6) << " 62 13 re f\n";
        c << pdfText("F2", 7.5, 1.0, 1.0, 1.0, sx + 12, sy - 2.5, "START: " + sn.id);
    }

    // D. Destination Pin (Ruby Red)
    if (pathIds.size() >= 2 && graph.nodes.count(pathIds.back())) {
        const auto& en = graph.nodes.at(pathIds.back());
        auto [ex, ey] = proj(en.lat, en.lng);
        // Outer halo
        c << "0.88 0.18 0.18 rg " << pdfCircle(ex, ey, 8.5, true, false);
        c << "1.5 w 1.0 1.0 1.0 RG 0.78 0.12 0.12 rg " << pdfCircle(ex, ey, 6.5, true, true);
        c << pdfText("F2", 7.0, 1.0, 1.0, 1.0, ex - 2.8, ey - 2.5, "E");
        // Dest Badge Label
        c << "0.78 0.12 0.12 rg " << (ex + 9) << " " << (ey - 6) << " 62 13 re f\n";
        c << pdfText("F2", 7.5, 1.0, 1.0, 1.0, ex + 12, ey - 2.5, "DEST: " + en.id);
    }

    // ── 6. Right Side Mission Manifest & Turn-by-Turn Panel ──────
    const double panelX = 515.0;
    const double panelW = 302.0;

    // Mission Route Card
    std::string sLabel = graph.nodes.count(startId) ? graph.nodes.at(startId).label : startId;
    std::string eLabel = graph.nodes.count(endId) ? graph.nodes.at(endId).label : endId;

    c << "0.96 0.97 0.99 rg 515 460 302 60 re f\n";
    c << "0.5 w 0.80 0.84 0.90 RG 515 460 302 60 re S\n";
    c << pdfText("F2", 8.0, 0.45, 0.50, 0.60, panelX + 12, 502, "OPERATIONAL ROUTE OBJECTIVE");
    c << pdfText("F2", 13.0, 0.10, 0.15, 0.25, panelX + 12, 482, sLabel + "  -->  " + eLabel);
    c << pdfText("F1", 7.5, 0.40, 0.45, 0.55, panelX + 12, 468, "Algorithm: " + algorithmLabel + " | Segments: " + std::to_string(pathIds.size() ? pathIds.size() - 1 : 0));

    // 4 Metrics Mini-Cards
    auto drawMetricCard = [&](double x, double y, double w, double h, const std::string& title, const std::string& val, const std::string& unit) {
        c << "1.0 1.0 1.0 rg " << x << " " << y << " " << w << " " << h << " re f\n";
        c << "0.4 w 0.85 0.88 0.92 RG " << x << " " << y << " " << w << " " << h << " re S\n";
        c << pdfText("F1", 6.5, 0.50, 0.55, 0.62, x + 6, y + h - 11, title);
        c << pdfText("F2", 11.0, 0.12, 0.20, 0.35, x + 6, y + 8, val);
        if (!unit.empty()) c << pdfText("F1", 7.0, 0.50, 0.55, 0.62, x + 6 + val.size() * 6.5 + 2, y + 8, unit);
    };

    double cardW = 68.0;
    double cardH = 34.0;
    double cardY = 416.0;

    std::ostringstream etaSs, distSs, riskSs;
    etaSs << (int)etaMinutes;
    distSs << std::fixed << std::setprecision(1) << totalDistanceKm;
    riskSs << std::fixed << std::setprecision(1) << cost;

    drawMetricCard(panelX, cardY, cardW, cardH, "ESTIMATED ETA", etaSs.str(), "min");
    drawMetricCard(panelX + 78, cardY, cardW, cardH, "TOTAL DISTANCE", distSs.str(), "km");
    drawMetricCard(panelX + 156, cardY, cardW, cardH, "HAZARD / RISK", hasCost ? riskSs.str() : "N/A", "");
    drawMetricCard(panelX + 234, cardY, cardW, cardH, "TOTAL STOPS", std::to_string(pathIds.size()), "nodes");

    // Turn-by-Turn Waypoints Table
    double tableY = 385.0;
    c << pdfText("F2", 9.0, 0.12, 0.20, 0.35, panelX, tableY, "TURN-BY-TURN WAYPOINT MANIFEST");
    tableY -= 14.0;

    // Table Header Bar
    c << "0.90 0.93 0.97 rg " << panelX << " " << tableY << " " << panelW << " 15 re f\n";
    c << pdfText("F2", 7.0, 0.25, 0.30, 0.40, panelX + 6, tableY + 4, "#");
    c << pdfText("F2", 7.0, 0.25, 0.30, 0.40, panelX + 24, tableY + 4, "TRANSIT LEG");
    c << pdfText("F2", 7.0, 0.25, 0.30, 0.40, panelX + 140, tableY + 4, "HIGHWAY / ROAD");
    c << pdfText("F2", 7.0, 0.25, 0.30, 0.40, panelX + 220, tableY + 4, "DIST");
    c << pdfText("F2", 7.0, 0.25, 0.30, 0.40, panelX + 258, tableY + 4, "CHECK");

    double rowY = tableY - 14.0;
    size_t maxLegs = std::min((size_t)12, pathIds.size() ? pathIds.size() - 1 : (size_t)0);

    for (size_t i = 0; i < maxLegs; i++) {
        std::string fromId = pathIds[i];
        std::string toId = pathIds[i+1];
        std::string fromName = graph.nodes.count(fromId) ? graph.nodes.at(fromId).label : fromId;
        std::string toName = graph.nodes.count(toId) ? graph.nodes.at(toId).label : toId;

        auto edgeInfo = graph.getEdgeInfo(fromId, toId);
        std::string roadName = edgeInfo ? edgeInfo->road : "Local Route";
        if (roadName.size() > 16) roadName = roadName.substr(0, 15) + ".";

        double dist = edgeInfo ? edgeInfo->distanceKm : 0.0;
        std::ostringstream dss;
        dss << (int)dist << " km";

        // Alternate row fill
        if (i % 2 == 1) {
            c << "0.97 0.98 0.99 rg " << panelX << " " << rowY << " " << panelW << " 13 re f\n";
        }
        c << "0.2 w 0.88 0.90 0.94 RG " << panelX << " " << rowY << " " << panelW << " 13 re S\n";

        // Row Text
        c << pdfText("F2", 6.5, 0.14, 0.44, 0.91, panelX + 6, rowY + 3.5, std::to_string(i + 1));
        std::string legDesc = fromName + " -> " + toName;
        if (legDesc.size() > 22) legDesc = legDesc.substr(0, 20) + "..";
        c << pdfText("F1", 6.5, 0.20, 0.22, 0.28, panelX + 24, rowY + 3.5, legDesc);
        c << pdfText("F1", 6.5, 0.40, 0.45, 0.52, panelX + 140, rowY + 3.5, roadName);
        c << pdfText("F2", 6.5, 0.20, 0.25, 0.35, panelX + 220, rowY + 3.5, dss.str());

        // Field Clearance Checkbox: [ ]
        c << "0.5 w 0.65 0.70 0.78 RG " << (panelX + 264) << " " << (rowY + 2.5) << " 8 8 re S\n";

        rowY -= 13.5;
    }

    if (pathIds.size() - 1 > maxLegs) {
        c << pdfText("F1", 6.0, 0.50, 0.55, 0.60, panelX + 6, rowY + 2.0, "... and " + std::to_string(pathIds.size() - 1 - maxLegs) + " more segments continuing to destination.");
        rowY -= 10.0;
    }

    // Operational Field Checklist Box
    double boxY = std::max(60.0, rowY - 60.0);
    c << "0.96 0.97 0.99 rg " << panelX << " " << boxY << " " << panelW << " 56 re f\n";
    c << "0.5 w 0.80 0.84 0.90 RG " << panelX << " " << boxY << " " << panelW << " 56 re S\n";
    c << pdfText("F2", 7.5, 0.15, 0.20, 0.30, panelX + 10, boxY + 44, "FIELD DISPATCH PROTOCOL CHECKLIST");
    c << "0.5 w 0.65 0.70 0.78 RG " << (panelX + 10) << " " << (boxY + 26) << " 7 7 re S\n";
    c << pdfText("F1", 6.5, 0.30, 0.35, 0.42, panelX + 22, boxY + 27, "Rapid Response Team Deployed & Radio Frequency Synced (VHF 152.4 MHz)");
    c << "0.5 w 0.65 0.70 0.78 RG " << (panelX + 10) << " " << (boxY + 12) << " 7 7 re S\n";
    c << pdfText("F1", 6.5, 0.30, 0.35, 0.42, panelX + 22, boxY + 13, "Landslide / Roadblock Status Re-checked via SDRF Control Room");

    // ── 7. Map Legend (Bottom of Map) ─────────────────────────────
    double legY = 36.0;
    c << "1.0 1.0 1.0 rg " << (mapX + 8) << " " << legY << " 459 20 re f\n";
    c << "0.4 w 0.85 0.88 0.92 RG " << (mapX + 8) << " " << legY << " 459 20 re S\n";

    // Legend item 1: Start
    c << "0.08 0.55 0.24 rg " << pdfCircle(mapX + 20, legY + 10, 4.0, true, false);
    c << pdfText("F2", 6.5, 0.25, 0.30, 0.38, mapX + 28, legY + 7.5, "Start Point");

    // Legend item 2: Active Route with Direction
    c << "3 w 0.12 0.38 0.88 RG " << (mapX + 85) << " " << (legY + 10) << " m " << (mapX + 115) << " " << (legY + 10) << " l S\n";
    c << "0.96 0.62 0.04 rg " << pdfArrow(mapX + 85, legY + 10, mapX + 115, legY + 10, 0.5, 4.5);
    c << pdfText("F2", 6.5, 0.25, 0.30, 0.38, mapX + 120, legY + 7.5, "Active Moving Path");

    // Legend item 3: Sequence Nodes
    c << "0.14 0.44 0.91 rg " << pdfCircle(mapX + 215, legY + 10, 3.5, true, false);
    c << pdfText("F2", 6.5, 0.25, 0.30, 0.38, mapX + 223, legY + 7.5, "Route Waypoints");

    // Legend item 4: Dest
    c << "0.78 0.12 0.12 rg " << pdfCircle(mapX + 300, legY + 10, 4.0, true, false);
    c << pdfText("F2", 6.5, 0.25, 0.30, 0.38, mapX + 308, legY + 7.5, "Destination");

    // Legend item 5: Network
    c << "1.0 w 0.80 0.84 0.90 RG " << (mapX + 370) << " " << (legY + 10) << " m " << (mapX + 395) << " " << (legY + 10) << " l S\n";
    c << pdfText("F2", 6.5, 0.25, 0.30, 0.38, mapX + 400, legY + 7.5, "Regional Road Network");

    // Footer Attribution
    c << pdfText("F1", 6.0, 0.50, 0.55, 0.62, 25, 12, "Uttarakhand Disaster Rescue System -- C++ High-Performance Graph Routing Engine | Map Data (c) OpenStreetMap contributors");
    c << pdfText("F1", 6.0, 0.50, 0.55, 0.62, 660, 12, "CONFIDENTIAL -- FIELD RESCUE USE ONLY");

    std::string contentStr = c.str();

    // ── 8. Assemble PDF Objects ───────────────────────────────────
    std::vector<std::string> objects;
    objects.push_back("<< /Type /Catalog /Pages 2 0 R >>");
    objects.push_back("<< /Type /Pages /Kids [3 0 R] /Count 1 >>");
    objects.push_back("<< /Type /Page /Parent 2 0 R /Resources << /Font << /F1 4 0 R /F2 5 0 R >> >> "
                      "/MediaBox [0 0 842 595] /Contents 6 0 R >>");
    objects.push_back("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>");
    objects.push_back("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold >>");

    std::ostringstream streamObj;
    streamObj << "<< /Length " << contentStr.size() << " >>\nstream\n" << contentStr << "\nendstream";
    objects.push_back(streamObj.str());

    std::ostringstream pdf;
    pdf << "%PDF-1.4\n";
    std::vector<size_t> offsets;
    for (size_t i = 0; i < objects.size(); i++) {
        offsets.push_back(pdf.str().size());
        pdf << (i + 1) << " 0 obj\n" << objects[i] << "\nendobj\n";
    }
    size_t xrefStart = pdf.str().size();
    pdf << "xref\n0 " << (objects.size() + 1) << "\n";
    pdf << "0000000000 65535 f \n";
    for (auto off : offsets) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%010zu 00000 n \n", off);
        pdf << buf;
    }
    pdf << "trailer\n<< /Size " << (objects.size() + 1) << " /Root 1 0 R >>\n";
    pdf << "startxref\n" << xrefStart << "\n%%EOF";

    return pdf.str();
}

// Backwards-compatible legacy signature
inline std::string buildRoutePdf(
    const std::string& startLabel, const std::string& endLabel,
    const std::string& algorithmLabel,
    const std::vector<std::string>& pathLabels,
    const std::vector<std::string>& roadNames,
    double etaMinutes, double totalDistanceKm, double cost, bool hasCost)
{
    Graph dummy;
    std::vector<std::string> dummyIds;
    for (size_t i = 0; i < pathLabels.size(); i++) {
        std::string id = "N" + std::to_string(i);
        dummy.addNode(id, pathLabels[i], "", 30.0 + i * 0.1, 78.5 + i * 0.1);
        dummyIds.push_back(id);
    }
    for (size_t i = 0; i + 1 < dummyIds.size(); i++) {
        std::string r = (i < roadNames.size()) ? roadNames[i] : "Route";
        dummy.addRoad(dummyIds[i], dummyIds[i+1], r, 20.0, 1.0, "medium");
    }
    std::string s = dummyIds.empty() ? startLabel : dummyIds.front();
    std::string e = dummyIds.empty() ? endLabel : dummyIds.back();
    return buildRoutePdf(dummy, s, e, algorithmLabel, dummyIds, etaMinutes, totalDistanceKm, cost, hasCost);
}
