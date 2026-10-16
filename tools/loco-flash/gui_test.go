package main

import (
	"bytes"
	"context"
	"encoding/json"
	"io"
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"strings"
	"testing"
	"time"
)

func TestWorkshopWindow(t *testing.T) {
	hexFile := encodeHex([]byte{0x0c, 0x94, 0x34, 0x00})
	src := memSource{
		releases: []Release{{
			Tag:         "controller",
			Name:        "Controller V2.1.0",
			Prerelease:  true,
			PublishedAt: time.Date(2026, 10, 13, 0, 0, 0, 0, time.UTC),
			Images: []Image{
				{Module: "controller", Version: "2.1.0", File: "controller_v2_1_0.hex", Protocol: "arduino", SHA256: "abc", URL: "https://github.com/loco-can/loco-can-software/releases/download/gui-test/controller_v2_1_0.hex"},
				{Module: "programmer", Version: "3.0.0", File: "programmer_v3_0_0.bin", Protocol: "esptool"},
			},
		}},
		body: hexFile,
	}
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	application, err := newApp(ctx, src, "loco-can/loco-can-software", "test-token")
	if err != nil {
		t.Fatal(err)
	}
	var got []byte
	application.flash = func(ctx context.Context, port string, data []byte, bauds []int, log flashLogger) error {
		got = append([]byte(nil), data...)
		log("Seite 1 von 1", 1, 1)
		return nil
	}
	application.ports = func() ([]Port, error) {
		return []Port{{Name: "/dev/ttyUSB0", USB: "0403:6001", Desc: "FTDI FT232R"}}, nil
	}
	application.identify = func(ctx context.Context, port string) (ModuleInfo, error) {
		if port != "/dev/ttyUSB0" {
			t.Fatalf("port %s", port)
		}
		on := true
		return ModuleInfo{Module: "controller", Version: "2.1.0", TypeID: 0x10, CAN: &on, Raw: "LOCO type=controller ver=2.1.0 id=0x10 can=1"}, nil
	}
	srv := httptest.NewServer(application.handler())
	defer srv.Close()

	page := getString(t, srv.URL+"/", "")
	if !strings.Contains(page, "Loco-CAN") || !strings.Contains(page, "test-token") || !strings.Contains(page, "Abfragen") {
		t.Fatalf("page missing title, token or probe: %.200s", page)
	}
	if i, j := strings.Index(page, `id="probe"`), strings.Index(page, `id="release"`); i < 0 || j < 0 || i > j {
		t.Fatalf("probe control must sit above the release list")
	}

	res, err := http.Get(srv.URL + "/api/ports")
	if err != nil {
		t.Fatal(err)
	}
	res.Body.Close()
	if res.StatusCode != http.StatusUnauthorized {
		t.Fatalf("status %d", res.StatusCode)
	}

	var listed struct {
		Releases []struct {
			Tag    string `json:"tag"`
			Images []struct {
				File     string `json:"file"`
				Protocol string `json:"protocol"`
			} `json:"images"`
		} `json:"releases"`
	}
	getJSON(t, srv.URL+"/api/releases", "test-token", &listed)
	if len(listed.Releases) != 1 || listed.Releases[0].Tag != "controller" || len(listed.Releases[0].Images) != 2 {
		t.Fatalf("releases %+v", listed.Releases)
	}

	var ports struct {
		Ports []struct {
			Name string `json:"name"`
		} `json:"ports"`
	}
	getJSON(t, srv.URL+"/api/ports", "test-token", &ports)
	if len(ports.Ports) != 1 || ports.Ports[0].Name != "/dev/ttyUSB0" {
		t.Fatalf("ports %+v", ports.Ports)
	}

	var probed struct {
		OK      bool   `json:"ok"`
		Module  string `json:"module"`
		Version string `json:"version"`
	}
	getJSON(t, srv.URL+"/api/probe?port=/dev/ttyUSB0", "test-token", &probed)
	if !probed.OK || probed.Module != "controller" || probed.Version != "2.1.0" {
		t.Fatalf("probe %+v", probed)
	}

	binRes := postJSON(t, srv.URL+"/api/flash", "test-token", map[string]any{
		"tag": "controller", "file": "programmer_v3_0_0.bin", "port": "/dev/ttyUSB0",
	})
	if binRes.StatusCode != http.StatusBadRequest {
		t.Fatalf("bin status %d", binRes.StatusCode)
	}
	binRes.Body.Close()

	started := postJSON(t, srv.URL+"/api/flash", "test-token", map[string]any{
		"tag": "controller", "file": "controller_v2_1_0.hex", "port": "/dev/ttyUSB0",
	})
	if started.StatusCode != http.StatusAccepted {
		body, _ := io.ReadAll(started.Body)
		t.Fatalf("flash %d %s", started.StatusCode, body)
	}
	started.Body.Close()

	var job jobView
	deadline := time.Now().Add(2 * time.Second)
	for time.Now().Before(deadline) {
		getJSON(t, srv.URL+"/api/job", "test-token", &job)
		if !job.Running && job.OK {
			break
		}
		time.Sleep(10 * time.Millisecond)
	}
	if !job.OK || job.Error != "" {
		t.Fatalf("job %+v", job)
	}
	if !bytes.Equal(got, hexFile) {
		t.Fatalf("flashed %d bytes", len(got))
	}
	if !strings.Contains(strings.Join(job.Lines, "\n"), "Seite 1 von 1") {
		t.Fatalf("log %v", job.Lines)
	}

	quitCalled := make(chan struct{}, 1)
	application.quit = func() { quitCalled <- struct{}{} }
	quit := postJSON(t, srv.URL+"/api/quit", "test-token", map[string]any{})
	quit.Body.Close()
	select {
	case <-quitCalled:
	case <-time.After(time.Second):
		t.Fatal("quit was not called")
	}
}

type memSource struct {
	releases []Release
	body     []byte
}

func (m memSource) Releases(context.Context, string, bool) ([]Release, error) {
	return m.releases, nil
}

func (m memSource) Download(_ context.Context, _ Image, dest string) error {
	if err := os.MkdirAll(filepath.Dir(dest), 0o755); err != nil {
		return err
	}
	return os.WriteFile(dest, m.body, 0o644)
}

func getString(t *testing.T, url, token string) string {
	t.Helper()
	req, err := http.NewRequest(http.MethodGet, url, nil)
	if err != nil {
		t.Fatal(err)
	}
	if token != "" {
		req.Header.Set("X-Loco-Token", token)
	}
	res, err := http.DefaultClient.Do(req)
	if err != nil {
		t.Fatal(err)
	}
	defer res.Body.Close()
	body, err := io.ReadAll(res.Body)
	if err != nil {
		t.Fatal(err)
	}
	return string(body)
}

func getJSON(t *testing.T, url, token string, dest any) {
	t.Helper()
	req, err := http.NewRequest(http.MethodGet, url, nil)
	if err != nil {
		t.Fatal(err)
	}
	req.Header.Set("X-Loco-Token", token)
	res, err := http.DefaultClient.Do(req)
	if err != nil {
		t.Fatal(err)
	}
	defer res.Body.Close()
	if res.StatusCode != http.StatusOK {
		body, _ := io.ReadAll(res.Body)
		t.Fatalf("%s %d %s", url, res.StatusCode, body)
	}
	if err := json.NewDecoder(res.Body).Decode(dest); err != nil {
		t.Fatal(err)
	}
}

func postJSON(t *testing.T, url, token string, body any) *http.Response {
	t.Helper()
	raw, err := json.Marshal(body)
	if err != nil {
		t.Fatal(err)
	}
	req, err := http.NewRequest(http.MethodPost, url, bytes.NewReader(raw))
	if err != nil {
		t.Fatal(err)
	}
	req.Header.Set("Content-Type", "application/json")
	req.Header.Set("X-Loco-Token", token)
	res, err := http.DefaultClient.Do(req)
	if err != nil {
		t.Fatal(err)
	}
	return res
}
