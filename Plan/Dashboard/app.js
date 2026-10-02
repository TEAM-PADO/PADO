(() => {
  'use strict';
  const data = window.PADO_DATA;
  const main = document.getElementById('main');
  if (!data) {
    main.innerHTML = '<div class="panel panel-body"><h1>자료를 불러오지 못했습니다.</h1><p class="page-description">같은 폴더에 dashboard-data.js가 있는지 확인하거나 자료_새로고침.cmd를 실행해 주세요.</p></div>';
    return;
  }
  const { review, meta } = data;
  const docs = new Map(data.documents.map(doc => [doc.path, doc]));
  const files = new Set(data.files);
  const repo = 'https://github.com/TEAM-PADO/PADO';
  const icons = {
    overview: '<rect x="3" y="3" width="7" height="7" rx="1.5"/><rect x="14" y="3" width="7" height="7" rx="1.5"/><rect x="3" y="14" width="7" height="7" rx="1.5"/><rect x="14" y="14" width="7" height="7" rx="1.5"/>',
    changes: '<path d="M4 7h12M13 3l4 4-4 4M20 17H8M11 13l-4 4 4 4"/>',
    tasks: '<rect x="4" y="4" width="16" height="17" rx="2"/><path d="M9 4V2h6v2M8 10h8M8 15h8"/>',
    decisions: '<path d="M9 18h6M10 21h4M8 14a7 7 0 1 1 8 0c-1 1-1 2-1 2H9s0-1-1-2z"/>',
    validation: '<path d="M12 2l8 4v6c0 5-8 10-8 10S4 17 4 12V6zM8 12l3 3 5-6"/>',
    library: '<path d="M3 5h7l2 2h9v13H3zM7 12h10M7 16h7"/>',
    search: '<circle cx="10.5" cy="10.5" r="6.5"/><path d="M16 16l5 5"/>',
    warning: '<path d="M12 3l10 18H2zM12 9v5M12 17v.1"/>',
    clock: '<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/>',
    doc: '<path d="M14 2H5v20h14V7zM14 2v6h5M8 12h8M8 16h8"/>'
  };
  const icon = name => `<svg viewBox="0 0 24 24" aria-hidden="true">${icons[name] || icons.doc}</svg>`;
  const escape = value => String(value ?? '').replace(/[&<>"']/g, char => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[char]));
  const badge = (label, tone = 'gray') => `<span class="badge ${escape(tone)}">${escape(label)}</span>`;
  const toneFor = state => ({ '기반 있음': 'blue', '도면 있음': 'blue', '미연결': 'amber', '구현 확인 필요': 'gray' }[state] || 'gray');
  const sourceButton = (path, label = '근거 문서 읽기') => `<button class="text-button" data-doc="${escape(path)}">${escape(label)}</button>`;
  const localURL = path => '../' + path.split('/').map(encodeURIComponent).join('/');
  const routes = [
    { id: 'overview', label: '한눈에 보기' }, { id: 'changes', label: '최근 변경' },
    { id: 'tasks', label: '작업·담당' }, { id: 'decisions', label: '기획 결정' },
    { id: 'validation', label: '통합 검증' }, { id: 'library', label: '문서·도면' }
  ];
  let currentRoute = 'overview';
  let filters = { query: '', owner: '', status: '', group: '' };
  let documentHistory = [];
  let activeDocument = '';
  let toastTimer;
  let pendingTestJump = '';
  const dialog = document.getElementById('document-dialog');
  let notes = {};
  let storageAvailable = true;
  try {
    const stored = JSON.parse(localStorage.getItem('pado-pm-notes-v1') || '{}');
    if (stored && typeof stored === 'object' && !Array.isArray(stored)) {
      for (const decision of review.decisions) {
        if (typeof stored[decision.id] === 'string') notes[decision.id] = stored[decision.id];
      }
    }
  } catch { storageAvailable = false; }
  function toast(message) {
    const element = document.getElementById('toast');
    element.textContent = message;
    element.classList.add('show');
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => element.classList.remove('show'), 3800);
  }
  function testsSummary() {
    const pass = data.tests.filter(test => /통과|성공/.test(test.result) && !/미통과|실패|미실행/.test(test.result)).length;
    const fail = data.tests.filter(test => /실패|미통과/.test(test.result)).length;
    const unrun = data.tests.filter(test => /미실행/.test(test.result)).length;
    return { pass, fail, unrun, other: data.tests.length - pass - fail - unrun, total: data.tests.length };
  }
  function header(title, description, action = '') {
    return `<div class="page-heading"><div><span class="eyebrow">PADO PROJECT DESK</span><h1>${escape(title)}</h1><p class="page-description">${escape(description)}</p></div>${action}</div>`;
  }
  function staleNotice() {
    if (!meta.changedSources.length && !meta.sinceReview.length && !meta.working.length) return '';
    const changed = [...new Set([...meta.changedSources, ...meta.sinceReview, ...meta.working])];
    return `<div class="notice">${icon('warning')}<div><strong>요약을 다시 검토할 변경이 있습니다.</strong><p>원문·Git 자료와 검토 요약의 기준이 다릅니다. 갱신을 요청한 뒤 구현 상태를 판단하세요.</p><details class="review-files"><summary>변경 ${changed.length}개 확인</summary><ul class="file-list">${changed.map(file => `<li>${escape(file)}</li>`).join('')}</ul></details></div></div>`;
  }
  function renderStat(label, value, suffix, detail, name, accent = false) {
    return `<article class="panel stat ${accent ? 'accent' : ''}"><span class="stat-label">${escape(label)}</span><span class="stat-icon">${icon(name)}</span><div class="stat-number">${escape(value)}<small>${escape(suffix)}</small></div><p class="subtext">${escape(detail)}</p></article>`;
  }
  function featureCards() {
    return review.features.map(feature => `<article class="panel feature-card"><div class="feature-top"><h3>${escape(feature.name)}</h3>${badge(feature.state, feature.tone)}</div><span class="owner">${escape(feature.owner)}</span><p>${escape(feature.current)}</p><div class="feature-next"><strong>다음 연결</strong>${escape(feature.next)}</div>${sourceButton(feature.source)}</article>`).join('');
  }
  function overview() {
    const summary = testsSummary();
    const decisions = [...review.decisions].sort((a, b) => (a.phase === '1단계' ? -1 : 0) - (b.phase === '1단계' ? -1 : 0)).slice(0, 3);
    return header('프로젝트 한눈에 보기', '지금 만드는 것, 남은 연결, 내가 판단할 일을 확인합니다.', '<a class="button primary" href="#decisions">결정할 일 확인</a>') + staleNotice() +
      `<div class="stats">${renderStat('작업 기준 단계', review.stage, '단계', review.stageConfirmed ? '팀이 확인한 현재 단계' : '문서상 1단계 범위 · 팀 확인 필요', 'tasks', true)}${renderStat('남은 기획 판단', review.decisions.length, '건', '제안·측정·플레이 결과가 필요한 항목', 'decisions')}${renderStat('통합 검증 통과', summary.pass, `/ ${summary.total}`, summary.unrun === summary.total ? '전체 미실행 · 실제 빌드 확인 필요' : `미실행 ${summary.unrun} · 실패 ${summary.fail} · 기타 ${summary.other}`, 'validation')}${renderStat('공유 기획 문서', data.documents.length, '개', '공용 기준과 담당별 자료 모음', 'library')}</div>
      <div class="overview-grid"><section class="panel stage-panel"><div class="panel-body"><span class="eyebrow">이번 통합 목표</span>${badge('1단계 작업 기준', 'blue')}<h2 class="stage-title">${escape(review.stageLabel)}</h2><p class="stage-goal">${escape(review.stageGoal)}</p><div class="stage-track">${review.stages.map(stage => `<div class="stage-step ${stage.id === review.stage ? 'current' : ''}"><small>STEP 0${stage.id}</small><strong>${escape(stage.title)}</strong><span>${escape(stage.description)}</span></div>`).join('')}</div><div class="stage-bottom"><p>완료 판정은 문서 작성 후 실제 빌드 검증으로 확인</p>${sourceButton('공용/01_개발순서.md', '단계별 범위 읽기')}</div></div></section>
      <section class="panel"><div class="panel-header"><h2>${icon('decisions')}먼저 판단할 일</h2><a class="text-button" href="#decisions">전체 ${review.decisions.length}건</a></div><div class="decision-list">${decisions.map(decision => `<button class="decision-mini" data-decision="${decision.id}"><div class="decision-mini-top"><span class="item-id">${decision.id}</span>${badge(decision.phase, 'blue')}</div><h3>${escape(decision.title)}</h3><p>${escape(decision.status)} · ${escape(decision.owner)}</p></button>`).join('')}</div></section></div>
      <div class="section-header"><h2>담당별 개발 현황</h2><a class="text-button" href="#tasks">작업 목록 보기</a></div><p class="context-note">${escape(review.basis)} 검토일 ${escape(review.reviewedAt)}.</p><div class="feature-grid">${featureCards()}</div>
      <div class="section-header"><h2>통합 전에 맞출 것</h2></div><div class="watch-grid">${review.watchpoints.map(point => `<article class="panel watch-card"><h3>${escape(point.title)}</h3><p>${escape(point.detail)}</p>${sourceButton(point.source)}</article>`).join('')}</div>`;
  }
  function toolbar({ owner = false, status = false, group = false, placeholder = '내용 검색' } = {}) {
    let selects = '';
    if (owner) selects += `<select id="owner-filter" aria-label="담당 파트"><option value="">전체 담당</option>${[...new Set(review.tasks.map(task => task.owner))].map(item => `<option>${escape(item)}</option>`).join('')}</select>`;
    if (status) selects += `<select id="status-filter" aria-label="상태"><option value="">전체 상태</option>${(currentRoute === 'validation' ? ['미실행', '통과', '실패', '기타'] : [...new Set(review.tasks.map(task => task.state))]).map(item => `<option>${escape(item)}</option>`).join('')}</select>`;
    if (group) selects += `<select id="group-filter" aria-label="문서 분류"><option value="">전체 분류</option>${[...new Set(data.documents.map(doc => doc.group))].map(item => `<option>${escape(item)}</option>`).join('')}</select>`;
    return `<div class="toolbar"><label class="search-box">${icon('search')}<input id="query-filter" type="search" aria-label="검색" placeholder="${escape(placeholder)}" autocomplete="off"></label>${selects}<span id="result-count" class="toolbar-result"></span></div>`;
  }
  const matchesQuery = item => !filters.query || JSON.stringify(item).toLocaleLowerCase('ko').includes(filters.query.toLocaleLowerCase('ko'));
  function taskRows() {
    const selected = review.tasks.filter(task => matchesQuery(task) && (!filters.owner || task.owner === filters.owner) && (!filters.status || task.state === filters.status));
    count(selected.length, review.tasks.length);
    if (!selected.length) return '<tr><td colspan="5" class="empty-state">조건에 맞는 작업이 없습니다. 검색어나 필터를 바꿔보세요.</td></tr>';
    return selected.map(task => `<tr><td><span class="item-id">${escape(task.id)}</span><div class="task-title">${escape(task.title)}</div>${sourceButton(task.source, '관련 기준')}<div>${task.verification.map(id => `<a class="test-ref" href="#validation" data-test-jump="${escape(id)}">${escape(id)}</a>`).join('')}</div></td><td>${escape(task.owner)}<p>개인 담당·목표일 미등록</p></td><td>${badge(task.state, toneFor(task.state))}</td><td class="dependency">${escape(task.dependency)}</td><td class="done">${escape(task.done)}</td></tr>`).join('');
  }
  function tasks() {
    return header('작업과 담당', '1단계 개발 범위를 확인 가능한 결과로 묶었습니다.', '<a class="button" href="#validation">완료 기준 확인</a>') + staleNotice() + toolbar({ owner: true, status: true, placeholder: '작업·담당·먼저 필요한 것 검색' }) +
      '<p class="context-note">기획 문서에서 정리한 작업 목록입니다. 실제 진행 상태·담당자·목표일은 팀 확인 후 갱신합니다.</p><section class="panel table-wrap"><table class="task-table"><thead><tr><th>작업</th><th>담당 파트</th><th>문서상 상태</th><th>먼저 필요한 것</th><th>완료 기준</th></tr></thead><tbody id="filtered-results"></tbody></table></section>';
  }
  function decisionCards() {
    const selected = review.decisions.filter(matchesQuery);
    count(selected.length, review.decisions.length);
    if (!selected.length) return '<div class="panel empty-state">검색에 맞는 결정 항목이 없습니다.</div>';
    return selected.map(decision => `<article class="panel decision-card"><header><span class="item-id">${decision.id}</span>${badge(decision.phase, 'blue')}${badge(decision.status, 'amber')}</header><h2>${escape(decision.title)}</h2><p class="decision-question">${escape(decision.question)}</p><div class="decision-section"><strong>현재 기준</strong>${escape(decision.basis)}</div><div class="decision-section"><strong>판단 전에 받을 자료</strong>${escape(decision.request)}</div><p class="decision-owner">${escape(decision.owner)} · 결정 시점 미정</p><div class="decision-actions">${sourceButton(decision.source)}<button class="button small" data-decision="${decision.id}">${notes[decision.id] ? '메모 이어쓰기' : '검토 메모'}</button></div>${notes[decision.id] ? '<span class="note-indicator">이 브라우저에 개인 메모가 있습니다.</span>' : ''}</article>`).join('');
  }
  function decisions() {
    return header('기획 결정', '문서에 남아 있는 질문과 판단에 필요한 자료를 모았습니다.', '<button class="button" data-action="export-notes">메모 내보내기</button>') + staleNotice() + toolbar({ placeholder: '결정할 내용·담당 검색' }) + '<p class="context-note">검토 메모는 개인 초안입니다. 확정한 내용은 원본 기획 문서에 반영해야 팀 기준이 됩니다.</p><div class="decision-grid" id="filtered-results"></div>';
  }
  function changes() {
    const newFiles = meta.sinceRefresh;
    return header('최근 변경', '이 작업 폴더에 받은 변경 기록과 검토 요약을 확인합니다.', '<button class="button" data-action="help">변경 후 갱신 방법</button>') + staleNotice() +
      `<p class="context-note">${meta.initial ? '첫 대시보드 작성 시점의 최근 이력입니다.' : `직전 자료 갱신 이후 변경 파일 ${newFiles.length}개.`} GitHub에서 아직 받지 않은 변경은 포함하지 않습니다.</p>
      ${newFiles.length ? `<details class="panel panel-body"><summary>직전 자료 갱신 이후 변경 파일 ${newFiles.length}개</summary><ul class="file-list">${newFiles.map(file => `<li>${escape(file)}</li>`).join('')}</ul></details>` : ''}
      <div class="overview-grid"><section class="panel"><div class="panel-header"><h2>최근 변경 기록</h2>${badge(`${data.commits.length}개 기록`)}</div><div class="commit-list">${data.commits.length ? data.commits.map(commit => `<article class="commit-item"><div class="commit-head"><h3>${escape(commit.subject)}</h3><a class="badge blue" href="${repo}/commit/${encodeURIComponent(commit.hash)}" target="_blank" rel="noopener noreferrer" aria-label="${escape(commit.short)} GitHub 변경 보기">${escape(commit.short)}</a></div><div class="commit-meta"><span>${escape(commit.date)}</span><span>${escape(commit.author)}</span><span>변경 파일 ${commit.files.length}개</span></div><details><summary>관련 파일 확인</summary><ul class="file-list">${commit.files.map(file => `<li>${repositoryFileLink(file, commit.hash)}</li>`).join('')}</ul></details></article>`).join('') : '<div class="empty-state">Git 기록을 읽을 수 없습니다. 원문 자료는 계속 볼 수 있습니다.</div>'}</div></section>
      <section><div class="section-header" style="margin-top:0"><h2>기획·PM 검토 요약</h2></div><p class="context-note">검토 기준 ${escape(review.reviewedAt)} · 원문 변경 시 재검토 필요</p><div class="highlights">${review.highlights.map(item => `<article class="panel highlight">${badge(item.kind, 'blue')}<h3>${escape(item.title)}</h3><p>${escape(item.detail)}</p>${sourceButton(item.source)}</article>`).join('')}</div></section></div>`;
  }
  function repositoryFileLink(file, revision = meta.head) {
    const planPath = file.startsWith('Plan/') ? file.slice(5) : null;
    if (planPath && docs.has(planPath)) return `<a href="#" data-doc="${escape(planPath)}">${escape(file)}</a>`;
    if (planPath && files.has(planPath)) return `<a href="${localURL(planPath)}" target="_blank" rel="noopener noreferrer">${escape(file)}</a>`;
    return `<a href="${repo}/blob/${encodeURIComponent(revision || 'HEAD')}/${file.split('/').map(encodeURIComponent).join('/')}" target="_blank" rel="noopener noreferrer">${escape(file)}</a>`;
  }
  const testTitles = { V01: '새 방 시작·지도 연결·안전 구역', V02: '권총으로 첫 차 구매 비용 벌기', V03: '첫 차 구매·승하차·로드킬', V04: '배달 수락·택배·타이머 시작', V05: '이동 중 목표·타이머 유지', V06: '배송·보상·다음 후보·저장', V07: '2인 공동 목표·같은 완료 보상', V08: '차량 동기화·중복 보상 방지', V09: '배달 완료 직후 종료·재개', V10: '야외에서 우체국 도착 시 저장', V11: '총 상점 출입은 저장하지 않음', V12: '4인 분산 플레이·공동 배송', V13: '공개 방·새 게임·이어하기·삭제', V14: '친구 방·스팀 초대 입장' };
  function testState(test) {
    if (/미실행/.test(test.result)) return '미실행';
    if (/미통과|실패/.test(test.result)) return '실패';
    if (/통과|성공/.test(test.result)) return '통과';
    return '기타';
  }
  function testCards() {
    const selected = data.tests.filter(test => matchesQuery({ ...test, title: testTitles[test.id] }) && (!filters.status || testState(test) === filters.status));
    count(selected.length, data.tests.length);
    if (!selected.length) return '<div class="panel empty-state">조건에 맞는 검증 항목이 없습니다.</div>';
    return selected.map(test => `<details class="panel test-item" id="test-${escape(test.id)}"><summary><span class="item-id">${escape(test.id)}</span><span class="test-title">${escape(testTitles[test.id] || test.steps)}</span><span class="test-players">${escape(test.players)}인</span>${badge(test.result, testState(test) === '통과' ? 'green' : testState(test) === '실패' ? 'red' : 'gray')}</summary><div class="test-body"><div class="decision-section"><strong>실행 절차</strong>${inline(test.steps)}</div><div class="decision-section"><strong>기대 결과</strong>${inline(test.expected)}</div><div class="decision-section"><strong>확인 담당</strong>${escape(test.owner)} · ${escape(test.players)}인</div><div class="decision-section">${sourceButton('공용/08_1단계_통합_검증표.md', '검증표와 결과 기록 기준 읽기')}</div></div></details>`).join('');
  }
  function validation() {
    const summary = testsSummary();
    return header('통합 검증', '문서에 기록된 실제 빌드 확인 결과로 완료 여부를 판단합니다.', sourceButton('공용/08_1단계_통합_검증표.md', '원본 검증표 읽기')) + staleNotice() +
      `<div class="stats">${renderStat('전체 검증', summary.total, '항목', '1인·2인·4인 실행 시나리오', 'tasks')}${renderStat('통과', summary.pass, '항목', '실제 실행 결과가 기록된 항목', 'validation', true)}${renderStat('미실행', summary.unrun, '항목', '기능이 있어도 실행 확인 필요', 'clock')}${renderStat('실패·기타', summary.fail + summary.other, '항목', `실패 ${summary.fail} · 기타 ${summary.other}`, 'warning')}</div>` + toolbar({ status: true, placeholder: '검증 항목·담당·기대 결과 검색' }) + '<p class="context-note">결과는 공용 검증표에서 읽습니다. 결과 변경 시 빌드·실행 날짜·인원·근거를 원본에 기록하세요.</p><div class="test-list" id="filtered-results"></div>';
  }
  function documentCards() {
    const selected = data.documents.filter(doc => matchesQuery(doc) && (!filters.group || doc.group === filters.group));
    count(selected.length, data.documents.length);
    if (!selected.length) return '<div class="panel empty-state">검색에 맞는 문서가 없습니다.</div>';
    return selected.map(doc => `<button class="panel doc-card" data-doc="${escape(doc.path)}"><span class="eyebrow">${escape(doc.group.replaceAll('_', ' '))}</span><h3>${escape(doc.title)}</h3><span class="doc-path">${escape(doc.path)}</span></button>`).join('');
  }
  function library() {
    const images = [
      { path: '레벨디자이너/도시_구조_블록아웃.png', title: '도시 구조 블록아웃', detail: '배달 동선과 도시 공간을 검토하는 개념 도면' },
      { path: '레벨디자이너/중앙_거점_블록아웃.png', title: '중앙 거점 확대', detail: '우체국·총 상점·차량 상점과 안전 경계 검토' }
    ].filter(image => files.has(image.path));
    return header('문서와 도면', '공용 기준과 담당별 자료를 검색하고 원문을 읽습니다.') + toolbar({ group: true, placeholder: '제목·본문 전체 검색' }) + '<div class="doc-grid" id="filtered-results"></div>' +
      `<div class="section-header"><h2>레벨 도면</h2>${sourceButton('레벨디자이너/도시_구조_블록아웃.md', '도면 설명 읽기')}</div><p class="context-note">현재 폴더에 있는 도면만 표시합니다. 클릭하면 확대해 볼 수 있습니다.</p><div class="map-grid">${images.length ? images.map(image => `<article class="panel map-card"><button data-image="${escape(image.path)}" data-title="${escape(image.title)}" aria-label="${escape(image.title)} 확대"><img src="${localURL(image.path)}" alt="${escape(image.title)}" loading="lazy"></button><div class="panel-body"><h3>${escape(image.title)}</h3><p>${escape(image.detail)}</p></div></article>`).join('') : '<div class="panel empty-state">현재 폴더에서 PNG 도면을 찾지 못했습니다.</div>'}</div>`;
  }
  function count(selected, total) {
    const element = document.getElementById('result-count');
    if (element) element.textContent = `${selected} / ${total}개`;
  }
  function updateResults() {
    const element = document.getElementById('filtered-results');
    if (!element) return;
    element.innerHTML = ({ tasks: taskRows, decisions: decisionCards, validation: testCards, library: documentCards }[currentRoute])();
  }
  function renderRoute() {
    const route = routes.find(route => route.id === location.hash.slice(1)) || routes[0];
    currentRoute = route.id;
    filters = { query: '', owner: '', status: '', group: '' };
    document.getElementById('breadcrumb').textContent = route.label;
    document.getElementById('navigation').innerHTML = routes.map(item => `<a class="nav-link ${item.id === route.id ? 'active' : ''}" href="#${item.id}" ${item.id === route.id ? 'aria-current="page"' : ''}>${icon(item.id)}${escape(item.label)}${item.id === 'decisions' ? `<span class="nav-count">${review.decisions.length}</span>` : ''}</a>`).join('');
    main.innerHTML = ({ overview, changes, tasks, decisions, validation, library }[route.id])();
    updateResults();
    document.title = `PADO · ${route.label}`;
    window.scrollTo({ top: 0, behavior: 'instant' });
    if (pendingTestJump && currentRoute === 'validation') {
      const test = document.getElementById('test-' + pendingTestJump);
      if (test) { test.open = true; test.scrollIntoView({ block: 'center' }); }
      pendingTestJump = '';
    }
  }

  // Small escaped Markdown reader: repository text cannot inject raw HTML or scripts.
  function resolveLink(target, sourcePath) {
    const cleaned = target.trim().replace(/^<|>$/g, '');
    if (/^https?:\/\//i.test(cleaned)) return { kind: 'external', href: cleaned };
    if (/^[a-z][a-z0-9+.-]*:/i.test(cleaned) || cleaned.startsWith('//')) return { kind: 'unsafe' };
    try {
      const base = new URL('https://pado.invalid/Plan/' + sourcePath.split('/').map(encodeURIComponent).join('/'));
      const url = new URL(cleaned, base);
      const path = decodeURIComponent(url.pathname.slice(1));
      if (path.startsWith('Plan/')) {
        const local = path.slice(5);
        if (docs.has(local)) return { kind: 'document', path: local, anchor: decodeURIComponent(url.hash.slice(1)) };
        if (files.has(local)) return { kind: 'asset', path: local, href: localURL(local) };
        return { kind: 'missing', path: local };
      }
      if (path.startsWith('Source/') || path.startsWith('Config/')) return { kind: 'external', href: repo + '/blob/' + encodeURIComponent(meta.head || 'HEAD') + '/' + path.split('/').map(encodeURIComponent).join('/') };
      return { kind: 'missing', path };
    } catch { return { kind: 'unsafe' }; }
  }
  function inline(text, sourcePath = activeDocument || 'README.md') {
    const pattern = /(`[^`]+`|!\[[^\]]*\]\([^)]+\)|\[[^\]]+\]\([^)]+\)|\*\*[^*]+\*\*)/g;
    let result = '', position = 0;
    for (const match of text.matchAll(pattern)) {
      result += escape(text.slice(position, match.index));
      const token = match[0];
      if (token.startsWith('`')) result += `<code>${escape(token.slice(1, -1))}</code>`;
      else if (token.startsWith('**')) result += `<strong>${inline(token.slice(2, -2), sourcePath)}</strong>`;
      else {
        const parsed = token.match(/^(!?)\[([^\]]*)\]\(([^)]+)\)$/);
        const [, imageMark, label, target] = parsed;
        const resolved = resolveLink(target, sourcePath);
        if (imageMark) {
          result += resolved.kind === 'asset' && /\.(png|jpe?g|webp|gif)$/i.test(resolved.path) ? `<img src="${escape(resolved.href)}" alt="${escape(label)}" loading="lazy">` : `<span class="missing-link">${escape(label || '이미지')} · 자료 없음</span>`;
        } else if (resolved.kind === 'document') result += `<a href="#" data-doc="${escape(resolved.path)}" data-anchor="${escape(resolved.anchor)}">${escape(label)}</a>`;
        else if (resolved.kind === 'asset' || resolved.kind === 'external') result += `<a href="${escape(resolved.href)}" target="_blank" rel="noopener noreferrer">${escape(label)}</a>`;
        else result += `<span class="missing-link" title="현재 폴더에서 자료를 찾지 못했습니다">${escape(label)}</span>`;
      }
      position = match.index + token.length;
    }
    return result + escape(text.slice(position));
  }
  function markdown(text, sourcePath) {
    const lines = text.replace(/\r/g, '').split('\n');
    let html = '', index = 0;
    const cells = line => line.trim().replace(/^\||\|$/g, '').split('|').map(cell => cell.trim());
    while (index < lines.length) {
      const line = lines[index];
      if (!line.trim()) { index++; continue; }
      if (/^```/.test(line)) {
        const code = []; index++;
        while (index < lines.length && !/^```/.test(lines[index])) code.push(lines[index++]);
        html += `<pre><code>${escape(code.join('\n'))}</code></pre>`; index++; continue;
      }
      const heading = line.match(/^(#{1,4})\s+(.+)$/);
      if (heading) { const level = heading[1].length; html += `<h${level}>${inline(heading[2], sourcePath)}</h${level}>`; index++; continue; }
      if (/^\s*\|/.test(line) && index + 1 < lines.length && /^\s*\|?[\s:|-]+\|\s*$/.test(lines[index + 1])) {
        const headerCells = cells(line); index += 2; let body = '';
        while (index < lines.length && /^\s*\|/.test(lines[index])) { body += '<tr>' + cells(lines[index++]).map(cell => `<td>${inline(cell, sourcePath)}</td>`).join('') + '</tr>'; }
        html += `<div class="table-wrap"><table><thead><tr>${headerCells.map(cell => `<th>${inline(cell, sourcePath)}</th>`).join('')}</tr></thead><tbody>${body}</tbody></table></div>`; continue;
      }
      if (/^>\s?/.test(line)) { html += `<blockquote>${inline(line.replace(/^>\s?/, ''), sourcePath)}</blockquote>`; index++; continue; }
      if (/^\s*[-*]\s+/.test(line) || /^\s*\d+\.\s+/.test(line)) {
        const ordered = /^\s*\d+\.\s+/.test(line), tag = ordered ? 'ol' : 'ul'; const regex = ordered ? /^\s*\d+\.\s+/ : /^\s*[-*]\s+/;
        html += `<${tag}>`;
        while (index < lines.length && regex.test(lines[index])) html += `<li>${inline(lines[index++].replace(regex, ''), sourcePath)}</li>`;
        html += `</${tag}>`; continue;
      }
      if (/^---+$/.test(line)) { html += '<hr>'; index++; continue; }
      html += `<p>${inline(line, sourcePath)}</p>`; index++;
    }
    return html;
  }
  function setDialog(title, eyebrow, body, toolbarHTML = '') {
    document.getElementById('dialog-title').textContent = title;
    document.getElementById('dialog-eyebrow').textContent = eyebrow;
    document.getElementById('dialog-content').className = 'dialog-content';
    document.getElementById('dialog-content').innerHTML = body;
    document.getElementById('dialog-toolbar').innerHTML = toolbarHTML;
    document.getElementById('dialog-toolbar').hidden = !toolbarHTML;
    if (!dialog.open) dialog.showModal();
    dialog.scrollTop = 0;
  }
  function openDocument(path, addHistory = true, anchor = '') {
    const doc = docs.get(path);
    if (!doc) { toast('현재 자료에서 이 문서를 찾지 못했습니다.'); return; }
    if (addHistory && activeDocument && activeDocument !== path) documentHistory.push(activeDocument);
    activeDocument = path;
    const back = documentHistory.length ? '<button class="button small" data-action="document-back">이전 문서</button>' : '';
    setDialog(doc.title, doc.group.replaceAll('_', ' '), `<article class="document-content">${markdown(doc.content, doc.path)}</article>`, `${back}<span>${escape(doc.path)}</span><a class="button small" href="${localURL(doc.path)}" target="_blank" rel="noopener noreferrer">원본 파일</a>`);
    if (anchor) {
      const normalize = value => value.toLowerCase().replace(/[^\p{L}\p{N}\s-]/gu, '').replace(/\s+/g, '-');
      const heading = [...dialog.querySelectorAll('.document-content h1,.document-content h2,.document-content h3,.document-content h4')].find(element => normalize(element.textContent) === anchor || normalize(element.textContent) === normalize(anchor));
      if (heading) heading.scrollIntoView({ block: 'center' });
    }
  }
  function openDecision(id) {
    const decision = review.decisions.find(item => item.id === id);
    if (!decision) return;
    activeDocument = '';
    setDialog(decision.title, `${decision.id} · ${decision.phase}`, `<p class="decision-question">${escape(decision.question)}</p><div class="decision-section"><strong>판단 전에 받을 자료</strong>${escape(decision.request)}</div><div class="decision-section">${sourceButton(decision.source)}</div><form class="note-form" id="note-form" data-id="${id}" style="margin-top:24px"><label for="decision-note">나의 검토 메모</label><textarea id="decision-note" placeholder="확인할 것, 선택한 방향, 팀에 요청할 내용을 적어두세요.">${escape(notes[id] || '')}</textarea><p>이 브라우저에만 남는 개인 초안입니다. 팀 기획에 반영하려면 메모를 내보낸 뒤 갱신을 요청하세요.</p>${!storageAvailable ? '<div class="notice">브라우저 저장이 제한되어 있습니다. 메모를 작성한 뒤 내보내기로 보관하세요.</div>' : ''}<div class="note-buttons"><button class="button primary" type="submit">메모 저장</button><button class="button" type="button" data-action="export-current-note" data-id="${id}">메모 내보내기</button></div></form>`, `<span>${escape(decision.owner)}</span>${badge(decision.status, 'amber')}`);
  }
  function saveNotes() {
    try { localStorage.setItem('pado-pm-notes-v1', JSON.stringify(notes)); storageAvailable = true; return true; }
    catch { storageAvailable = false; return false; }
  }
  function exportNotes(currentId = '') {
    if (currentId) notes[currentId] = document.getElementById('decision-note')?.value || '';
    const selected = review.decisions.filter(decision => notes[decision.id]?.trim() && (!currentId || currentId === decision.id));
    if (!selected.length) { toast('내보낼 메모가 없습니다. 결정 항목에 메모를 먼저 작성하세요.'); return; }
    const text = '# PADO 기획 검토 메모\n\n> 개인 초안입니다. 원본 기획에 반영하기 전까지 확정 사항이 아닙니다.\n\n' + selected.map(decision => `## ${decision.id} ${decision.title}\n\n${notes[decision.id]}\n\n근거: Plan/${decision.source}\n`).join('\n');
    const url = URL.createObjectURL(new Blob(['\ufeff', text], { type: 'text/markdown;charset=utf-8' }));
    const link = document.createElement('a'); link.href = url; link.download = 'PADO_기획검토_메모.md'; document.body.append(link); link.click(); link.remove();
    setTimeout(() => URL.revokeObjectURL(url), 5000); toast('검토 메모를 파일로 내보냈습니다.');
  }
  function help() {
    activeDocument = '';
    setDialog('팀 변경을 받은 뒤 사용하는 방법', '대시보드 사용 안내', `<div class="help-steps"><div class="help-step"><span class="number">1</span><div><strong>GitHub에서 팀 변경을 받습니다.</strong><p>이 화면은 현재 작업 폴더에 받은 자료를 보여줍니다.</p></div></div><div class="help-step"><span class="number">2</span><div><strong>아래 문장으로 갱신을 요청합니다.</strong><p>변경 내용을 검토해 현황과 판단할 일을 함께 업데이트합니다.</p></div></div></div><div class="prompt-box" id="refresh-prompt">팀 변경 받았어. 지난 검토 이후 변경을 확인해서 Plan 대시보드의 현황·결정할 일·검증 결과를 갱신해줘.</div><button class="button primary" data-action="copy-prompt">갱신 요청 복사</button><div class="help-steps"><div class="help-step"><span class="number">3</span><div><strong>브라우저를 새로고침하고 판단합니다.</strong><p>최근 변경 → 기획 결정 → 작업·담당 순으로 확인하세요. 원문은 화면 안에서 읽을 수 있습니다.</p></div></div></div><div class="decision-section"><strong>원문과 변경 기록만 다시 가져오려면</strong>같은 폴더의 ‘자료_새로고침.cmd’를 실행한 뒤 브라우저를 새로고침하세요. 상태 요약은 검토 후 갱신해야 합니다.</div><div class="decision-section"><strong>검토 메모</strong>메모는 개인 초안으로 이 브라우저에만 저장됩니다. 내보내기 파일을 전달하면 원본 기획에 반영할 수 있습니다.</div>`, `<span>요약 검토 ${escape(review.reviewedAt)} · 자료 갱신 ${escape(meta.generatedAt)}</span>`);
  }
  document.addEventListener('click', async event => {
    const docButton = event.target.closest('[data-doc]');
    if (docButton) { event.preventDefault(); openDocument(docButton.dataset.doc, true, docButton.dataset.anchor || ''); return; }
    const decisionButton = event.target.closest('[data-decision]');
    if (decisionButton) { openDecision(decisionButton.dataset.decision); return; }
    const imageButton = event.target.closest('[data-image]');
    if (imageButton) {
      activeDocument = '';
      setDialog(imageButton.dataset.title, '레벨 도면', `<img src="${localURL(imageButton.dataset.image)}" alt="${escape(imageButton.dataset.title)}">`, `<a class="button small" href="${localURL(imageButton.dataset.image)}" target="_blank" rel="noopener noreferrer">원본 크기로 보기</a>`);
      document.getElementById('dialog-content').classList.add('image-preview'); return;
    }
    const testJump = event.target.closest('[data-test-jump]');
    if (testJump) {
      event.preventDefault();
      pendingTestJump = testJump.dataset.testJump;
      if (location.hash === '#validation') renderRoute();
      else location.hash = 'validation';
      return;
    }
    const action = event.target.closest('[data-action]');
    if (!action) return;
    switch (action.dataset.action) {
      case 'help': help(); break;
      case 'close-dialog': dialog.close(); break;
      case 'document-back': { const previous = documentHistory.pop(); if (previous) openDocument(previous, false); break; }
      case 'export-notes': exportNotes(); break;
      case 'export-current-note': exportNotes(action.dataset.id); break;
      case 'copy-prompt': {
        const text = document.getElementById('refresh-prompt').textContent;
        try { await navigator.clipboard.writeText(text); toast('갱신 요청을 복사했습니다.'); }
        catch { const range = document.createRange(); range.selectNodeContents(document.getElementById('refresh-prompt')); const selection = window.getSelection(); selection.removeAllRanges(); selection.addRange(range); toast('문장을 선택했습니다. Ctrl+C로 복사하세요.'); }
        break;
      }
    }
  });
  document.addEventListener('input', event => {
    if (event.target.id === 'query-filter') { filters.query = event.target.value; updateResults(); }
  });
  document.addEventListener('change', event => {
    if (event.target.id === 'owner-filter') filters.owner = event.target.value;
    else if (event.target.id === 'status-filter') filters.status = event.target.value;
    else if (event.target.id === 'group-filter') filters.group = event.target.value;
    else return;
    updateResults();
  });
  document.addEventListener('submit', event => {
    if (event.target.id !== 'note-form') return;
    event.preventDefault();
    notes[event.target.dataset.id] = document.getElementById('decision-note').value;
    const saved = saveNotes();
    toast(saved ? '이 브라우저에 검토 메모를 저장했습니다.' : '브라우저 저장이 제한되어 있습니다. 메모를 내보내서 보관하세요.');
    if (saved) { dialog.close(); if (currentRoute === 'decisions') updateResults(); }
  });
  dialog.addEventListener('close', () => { activeDocument = ''; documentHistory = []; });
  dialog.addEventListener('click', event => { if (event.target === dialog) { const rect = dialog.getBoundingClientRect(); if (event.clientX < rect.left || event.clientX > rect.right || event.clientY < rect.top || event.clientY > rect.bottom) dialog.close(); } });
  window.addEventListener('hashchange', renderRoute);
  document.getElementById('revision').textContent = meta.gitAvailable ? `기준 ${meta.head.slice(0, 7)}` : 'Git 기록 없음';
  document.getElementById('updated-at').textContent = `자료 갱신 ${meta.generatedAt} · 한국 시간`;
  renderRoute();
})();
